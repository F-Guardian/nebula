// Copyright (c) 2020 vesoft inc. All rights reserved.
//
// This source code is licensed under Apache 2.0 License.

#include "graph/executor/stream/StreamCollectExecutor.h"

namespace nebula {
namespace graph {

std::shared_ptr<RoundResult> StreamCollectExecutor::executeOneRound(
  std::shared_ptr<DataSet> input, std::string offset) {
    std::cout << "input: " << input << ", offset: " << offset << std::endl;
    DLOG(INFO) << "StreamCollectExecutor executeOneRound.";
    std::lock_guard<std::mutex> lock(mutex_);
    finalDataSet_.append(std::move(*input));
    DLOG(INFO) << "finalDataSet_ appended.";
    return std::make_shared<RoundResult>(nullptr, false, "");
}

void StreamCollectExecutor::markFinishExecutor() {
    DLOG(INFO) << "StreamCollectExecutor markFinishExecutor.";
    auto status = finish(ResultBuilder().value(Value(std::move(finalDataSet_))).build());
    if (rootPromiseHasBeenSet_) {
        rootPromise_.setValue(status);
    }
}

}  // namespace graph
}  // namespace nebula
