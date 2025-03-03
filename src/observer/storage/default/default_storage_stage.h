/* Copyright (c) 2021 Xie Meiyi(xiemeiyi@hust.edu.cn) and OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

//
// Created by Meiyi & Longda on 2021/4/13.
//

#pragma once

#include "common/seda/stage.h"
#include "common/metrics/metrics.h"

class DefaultHandler;
class SqlResult;

class DefaultStorageStage : public common::Stage {
public:
  ~DefaultStorageStage();
  static Stage *make_stage(const std::string &tag);

protected:
  // common function
  DefaultStorageStage(const char *tag);
  bool set_properties() override;

  bool initialize() override;
  void cleanup() override;
  void handle_event(common::StageEvent *event) override;
  void callback_event(common::StageEvent *event, common::CallbackContext *context) override;

private:
  void load_data(const char *db_name, const char *table_name, const char *file_name, SqlResult *sql_result);

protected:
  common::SimpleTimer *query_metric_ = nullptr;
  static const std::string QUERY_METRIC_TAG;

private:
  DefaultHandler *handler_;
};
