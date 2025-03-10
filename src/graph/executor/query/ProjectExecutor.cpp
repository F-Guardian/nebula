// Copyright (c) 2020 vesoft inc. All rights reserved.
//
// This source code is licensed under Apache 2.0 License.

#include "graph/executor/query/ProjectExecutor.h"

#include "graph/planner/plan/Query.h"
#include "graph/service/GraphFlags.h"

namespace nebula {
namespace graph {

folly::Future<Status> ProjectExecutor::execute() {
  // throw std::bad_alloc in MemoryCheckGuard verified
  SCOPED_TIMER(&execTime_);
  auto *project = asNode<Project>(node());
  auto iter = ectx_->getResult(project->inputVar()).iter();
  DCHECK(!!iter);

  if (FLAGS_max_job_size <= 1) {
    auto ds = handleJob(0, iter->size(), iter.get());
    return finish(ResultBuilder().value(Value(std::move(ds))).build());
  } else {
    DataSet ds;
    ds.colNames = project->colNames();
    ds.rows.reserve(iter->size());

    auto scatter = [this](size_t begin, size_t end, Iterator *tmpIter) -> StatusOr<DataSet> {
      return handleJob(begin, end, tmpIter);
    };

    auto gather = [this, result = std::move(ds)](
                      std::vector<folly::Try<StatusOr<DataSet>>> &&results) mutable {
      memory::MemoryCheckGuard guard;
      for (auto &respVal : results) {
        if (respVal.hasException()) {
          auto ex = respVal.exception().get_exception<std::bad_alloc>();
          if (ex) {
            throw std::bad_alloc();
          } else {
            throw std::runtime_error(respVal.exception().what().c_str());
          }
        }
        auto res = std::move(respVal).value();
        auto &&rows = std::move(res).value();
        result.rows.insert(result.rows.end(),
                           std::make_move_iterator(rows.begin()),
                           std::make_move_iterator(rows.end()));
      }
      finish(ResultBuilder().value(Value(std::move(result))).build());
      return Status::OK();
    };

    return runMultiJobs(std::move(scatter), std::move(gather), iter.get());
  }
}

DataSet ProjectExecutor::handleJob(size_t begin, size_t end, Iterator *iter) {
  auto *project = asNode<Project>(node());
  auto columns = project->columns()->clone();
  DataSet ds;
  ds.colNames = project->colNames();
  QueryExpressionContext ctx(qctx()->ectx());
  ds.rows.reserve(end - begin);
  for (; iter->valid() && begin++ < end; iter->next()) {
    Row row;
    for (auto &col : columns->columns()) {
      Value val = col->expr()->eval(ctx(iter));
      row.values.emplace_back(std::move(val));
    }
    ds.rows.emplace_back(std::move(row));
  }
  return ds;
}

}  // namespace graph
}  // namespace nebula
