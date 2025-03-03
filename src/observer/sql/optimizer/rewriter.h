/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

//
// Created by Wangyunlai on 2022/12/29.
//

#pragma once

#include <memory>
#include <vector>
#include "sql/optimizer/rewrite_rule.h"

class LogicalOperator;

class Rewriter 
{
public:
  Rewriter();
  virtual ~Rewriter() = default;

  RC rewrite(std::unique_ptr<LogicalOperator> &oper, bool &change_made);

private:
  std::vector<std::unique_ptr<RewriteRule>> rewrite_rules_;
};
