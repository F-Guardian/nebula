// Copyright (c) 2020 vesoft inc. All rights reserved.
//
// This source code is licensed under Apache 2.0 License.

#include "graph/executor/stream/LimitStreamExecutor.h"

namespace nebula {
namespace graph {

std::shared_ptr<RoundResult> LimitStreamExecutor::executeOneRound(
  std::shared_ptr<DataSet> input, std::unordered_map<Value, nebula::storage::cpp2::ScanCursor> offset) {
    std::cout << "input: " << input << std::endl;
    auto ds = std::make_shared<nebula::DataSet>();
    ds->colNames = input->colNames;
    for (auto row : input->rows) {
        if (++counter_ <= limit_) {
            ds->rows.emplace_back(std::move(row));
        } else {
            DLOG(INFO) << "Exceed the limit number, ignore this row";
            markStopExecutor();
        }
    }
    return std::make_shared<RoundResult>(ds, false, offset);
}

}  // namespace graph
}  // namespace nebula
