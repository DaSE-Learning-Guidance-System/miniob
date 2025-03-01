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
// Created by Wangyunlai on 2021/5/24.
//

#include <atomic>

#include "storage/trx/trx.h"
#include "storage/common/table.h"
#include "storage/record/record_manager.h"
#include "storage/common/field_meta.h"
#include "common/log/log.h"
#include "storage/common/field.h"
#include "storage/trx/mvcc_trx.h"
#include "storage/trx/vacuous_trx.h"

static TrxKit *global_trxkit = nullptr;

TrxKit *TrxKit::create(const char *name)
{
  if (0 == strcasecmp(name, "mvcc")) {
    return new MvccTrxKit();
  }
  
  return new VacuousTrxKit();
}

RC TrxKit::init_global(const char *name)
{
  ASSERT(global_trxkit == nullptr, "init global trx kit twice");
  TrxKit *trx_kit = create(name);
  if (nullptr == trx_kit) {
    LOG_ERROR("failed to create trx kit by name. name=%s", name);
    return RC::INTERNAL;
  }

  RC rc = trx_kit->init();
  if (rc == RC::SUCCESS) {
    global_trxkit = trx_kit;
  }
  return rc;
}

TrxKit *TrxKit::instance()
{
  return global_trxkit;
}