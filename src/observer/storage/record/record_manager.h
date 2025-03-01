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

#include <sstream>
#include <limits>
#include "storage/default/disk_buffer_pool.h"
#include "storage/trx/latch_memo.h"
#include "storage/record/record.h"
#include "common/lang/bitmap.h"

class ConditionFilter;
class RecordPageHandler;
class Trx;
class Table;

/**
 * 数据文件，按照页面来组织，每一页都存放一些记录/数据行
 * 每一页都有一个这样的页头，虽然看起来浪费，但是现在就简单的这么做
 * 从这个页头描述的信息来看，当前仅支持定长行/记录。如果要支持变长记录，
 * 或者超长（超出一页）的记录，这么做是不合适的。
 */
struct PageHeader 
{
  int32_t record_num;           // 当前页面记录的个数
  int32_t record_capacity;      // 最大记录个数
  int32_t record_real_size;     // 每条记录的实际大小
  int32_t record_size;          // 每条记录占用实际空间大小(可能对齐)
  int32_t first_record_offset;  // 第一条记录的偏移量
};

/**
 * 遍历一个页面中每条记录的iterator
 */
class RecordPageIterator 
{
public:
  RecordPageIterator();
  ~RecordPageIterator();

  void init(RecordPageHandler &record_page_handler, SlotNum start_slot_num = 0);

  bool has_next();
  RC next(Record &record);

  bool is_valid() const
  {
    return record_page_handler_ != nullptr;
  }

private:
  RecordPageHandler *record_page_handler_ = nullptr;
  PageNum page_num_ = BP_INVALID_PAGE_NUM;
  common::Bitmap bitmap_;
  SlotNum next_slot_num_ = 0;
};

/**
 * 负责处理一个页面中各种操作，比如插入记录、删除记录或者查找记录
 */
class RecordPageHandler 
{
public:
  RecordPageHandler() = default;
  ~RecordPageHandler();
  RC init(DiskBufferPool &buffer_pool, PageNum page_num, bool readonly);
  RC recover_init(DiskBufferPool &buffer_pool, PageNum page_num);
  RC init_empty_page(DiskBufferPool &buffer_pool, PageNum page_num, int record_size);
  RC cleanup();

  RC insert_record(const char *data, RID *rid);
  RC recover_insert_record(const char *data, RID *rid);

  RC delete_record(const RID *rid);

  RC get_record(const RID *rid, Record *rec);

  PageNum get_page_num() const;

  bool is_full() const;

protected:
  char *get_record_data(SlotNum slot_num)
  {
    return frame_->data() + page_header_->first_record_offset + (page_header_->record_size * slot_num);
  }

protected:
  DiskBufferPool *disk_buffer_pool_ = nullptr;
  bool readonly_ = false;
  Frame *frame_ = nullptr;
  PageHeader *page_header_ = nullptr;
  char *bitmap_ = nullptr;

private:
  friend class RecordPageIterator;
};

class RecordFileHandler 
{
public:
  RecordFileHandler() = default;
  ~RecordFileHandler();
  
  RC init(DiskBufferPool *buffer_pool);
  void close();

  /**
   * 更新指定文件中的记录，rec指向的记录结构中的rid字段为要更新的记录的标识符，
   * pData字段指向新的记录内容
   */
  RC update_record(const Record *rec);

  /**
   * 从指定文件中删除标识符为rid的记录
   */
  RC delete_record(const RID *rid);

  /**
   * 插入一个新的记录到指定文件中，data为指向新纪录内容的指针，返回该记录的标识符rid
   */
  RC insert_record(const char *data, int record_size, RID *rid);
  RC recover_insert_record(const char *data, int record_size, RID *rid);

  /**
   * 获取指定文件中标识符为rid的记录内容到rec指向的记录结构中
   */
  RC get_record(RecordPageHandler &page_handler, const RID *rid, bool readonly, Record *rec);

  RC visit_record(const RID &rid, bool readonly, std::function<void(Record &)> visitor);

private:
  RC init_free_pages();

private:
  DiskBufferPool *disk_buffer_pool_ = nullptr;
  std::unordered_set<PageNum> free_pages_;  // 没有填充满的页面集合
  common::Mutex lock_; // 当编译时增加-DCONCURRENCY=ON 选项时，才会真正的支持并发
};

class RecordFileScanner 
{
public:
  RecordFileScanner() = default;
  ~RecordFileScanner();

  /**
   * 打开一个文件扫描。
   * 如果条件不为空，则要对每条记录进行条件比较，只有满足所有条件的记录才被返回
   * @param table        遍历的哪张表
   * @param buffer_pool  访问的文件
   * @param readonly     当前是否只读操作。访问数据时，需要对页面加锁。比如
   *                     删除时也需要遍历找到数据，然后删除，这时就需要加写锁
   * @param condition_filter 做一些初步过滤操作
   */
  RC open_scan(Table *table, 
               DiskBufferPool &buffer_pool, 
               Trx *trx, 
               bool readonly, 
               ConditionFilter *condition_filter);

  /**
   * 关闭一个文件扫描，释放相应的资源
   */
  RC close_scan();

  bool has_next();
  RC next(Record &record);

private:
  RC fetch_next_record();
  RC fetch_next_record_in_page();

private:
  Table *              table_ = nullptr;
  DiskBufferPool *     disk_buffer_pool_ = nullptr;
  Trx *                trx_ = nullptr;
  bool                 readonly_ = false;  // 遍历出来的数据，是否可能对它做修改

  BufferPoolIterator   bp_iterator_;    // 遍历buffer pool的所有页面
  ConditionFilter *    condition_filter_ = nullptr; // 过滤record
  RecordPageHandler    record_page_handler_; 
  RecordPageIterator   record_page_iterator_; // 遍历某个页面上的所有record
  Record               next_record_;
};
