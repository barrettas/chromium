// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_FEEDBACK_FEEDBACK_COMMON_H_
#define CHROME_BROWSER_FEEDBACK_FEEDBACK_COMMON_H_

#include <map>
#include <string>

#include "base/file_util.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/scoped_vector.h"
#include "base/synchronization/lock.h"

namespace userfeedback {
class ExtensionSubmit;
}

namespace feedback_util {
bool ZipString(const base::FilePath& filename,
               const std::string& data,
               std::string* compressed_data);
}

// This is the base class for FeedbackData. It primarily knows about
// data common to all feedback reports and how to zip things.
class FeedbackCommon : public base::RefCountedThreadSafe<FeedbackCommon> {
 public:
  typedef std::map<std::string, std::string> SystemLogsMap;

  struct AttachedFile {
    explicit AttachedFile(const std::string& filename);

    std::string name;
    std::string data;
  };

  // Determine if the given feedback value is small enough to not need to
  // be compressed.
  static bool BelowCompressionThreshold(const std::string& content);

  FeedbackCommon();

  void CompressFile(const base::FilePath& filename,
                    const std::string& zipname,
                    scoped_ptr<std::string> data);
  void AddFile(const std::string& filename, scoped_ptr<std::string> data);

  void AddLog(const std::string& name, const std::string& value);
  void AddLogs(scoped_ptr<SystemLogsMap> logs);
  void CompressLogs();

  void AddFilesAndLogsToReport(userfeedback::ExtensionSubmit* feedback_data);

  // Fill in |feedback_data| with all the data that we have collected.
  // CompressLogs() must have already been called.
  void PrepareReport(userfeedback::ExtensionSubmit* feedback_data);

  // Getters
  const std::string& category_tag() const { return category_tag_; }
  const std::string& page_url() const { return page_url_; }
  const std::string& description() const { return description_; }
  const std::string& user_email() const { return user_email_; }
  std::string* image() const { return image_.get(); }
  SystemLogsMap* sys_info() const { return logs_.get(); }

  const AttachedFile* attachment(size_t i) const { return attachments_[i]; }
  size_t attachments() const { return attachments_.size(); }

  // Setters
  void set_category_tag(const std::string& category_tag) {
    category_tag_ = category_tag;
  }
  void set_page_url(const std::string& page_url) { page_url_ = page_url; }
  void set_description(const std::string& description) {
    description_ = description;
  }
  void set_user_email(const std::string& user_email) {
    user_email_ = user_email;
  }
  void set_image(scoped_ptr<std::string> image) { image_ = image.Pass(); }

 protected:
  friend class base::RefCountedThreadSafe<FeedbackCommon>;
  friend class FeedbackCommonTest;

  virtual ~FeedbackCommon();

 private:
  std::string category_tag_;
  std::string page_url_;
  std::string description_;
  std::string user_email_;

  scoped_ptr<std::string> image_;

  // It is possible that multiple attachment add calls are running in
  // parallel, so synchronize access.
  base::Lock attachments_lock_;
  ScopedVector<AttachedFile> attachments_;

  scoped_ptr<SystemLogsMap> logs_;
};

#endif  // CHROME_BROWSER_FEEDBACK_FEEDBACK_COMMON_H_
