// Copyright 2016 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.!

#include "word_model_trainer.h"

#include <cmath>

#include "stringpiece.h"
#include "util.h"
#include "word_model.h"

namespace sentencepiece {
namespace word {

void Trainer::Train() {
#define CHECK_RANGE(variable, minval, maxval) \
  CHECK(variable >= minval && variable <= maxval)

  CHECK_GT(trainer_spec_.input().size(), 0);
  CHECK(!trainer_spec_.model_prefix().empty());
  CHECK_RANGE(trainer_spec_.character_coverage(), 0.98, 1.0);
  CHECK_RANGE(trainer_spec_.input_sentence_size(), 100, 100000000);
  CHECK_GT(trainer_spec_.vocab_size(), 0);
#undef CHECK_RANGE

  LOG(INFO) << "Starts training with : \n" << trainer_spec_.Utf8DebugString();

  CHECK(normalizer_spec_.escape_whitespaces());
  CHECK_EQ(TrainerSpec::WORD, trainer_spec_.model_type());

  LoadSentences();

  std::unordered_map<std::string, uint64> freq;
  for (const auto &it : sentences_) {
    for (const auto &s : SplitIntoWords(it.first)) {
      freq[s.to_string()] += it.second;
    }
  }
  const int meta_symbols_size = trainer_spec_.control_symbols().size() +
                                trainer_spec_.user_defined_symbols().size() +
                                3;  // <s>, </s>, <unk>

  const int vocab_size = trainer_spec_.vocab_size() - meta_symbols_size;
  CHECK_GE(vocab_size, 0);

  uint64 sum = 0;
  for (const auto &it : freq) {
    sum += it.second;
  }

  const float logsum = log(sum);

  CHECK(final_pieces_.empty());
  for (const auto &it : Sorted(freq)) {
    if (it.first.find(kUNKStr) != std::string::npos) {
      continue;
    }
    if (final_pieces_.size() == static_cast<size_t>(vocab_size)) {
      break;
    }
    final_pieces_.emplace_back(it.first, log(it.second) - logsum);
  }

  Save();
}
}  // namespace word
}  // namespace sentencepiece
