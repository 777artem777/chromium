// Copyright 2010 Google Inc. All Rights Reserved.
// Author: rkc@google.com (Rahul Chaturvedi)

#ifndef CHROME_BROWSER_BUG_REPORT_DATA_H_
#define CHROME_BROWSER_BUG_REPORT_DATA_H_


#include <string>
#include <vector>

#include "base/utf_string_conversions.h"
#include "chrome/browser/bug_report_util.h"

#if defined(OS_CHROMEOS)
#include "chrome/browser/chromeos/cros/syslogs_library.h"
#endif

class BugReportData {
 public:
  // Make sure we initialize these flags to false since SyslogsComplete
  // may be triggered before we've called update data; in which case,
  // we do not want it to just delete the logs it just gathered, and we
  // don't want it to send the report either - this will make sure that if
  // SyslogsComplete gets called before UpdateData, we'll simply populate the
  // sys_info and zip_content fields and exit without disturbing anything else
  BugReportData() : profile_(NULL)
#if defined(OS_CHROMEOS)
                    , sent_report_(false), send_sys_info_(false)
#endif
                    {
  }

  // Defined in bug_report_ui.cc
  void SendReport();

  void UpdateData(Profile* profile
                  , const std::string& target_tab_url
                  , const string16& target_tab_title
                  , const int problem_type
                  , const std::string& page_url
                  , const std::string& description
                  , const std::vector<unsigned char>& image
#if defined(OS_CHROMEOS)
                  , const std::string& user_email
                  , const bool send_sys_info
                  , const bool sent_report
#endif
                  ) {
    profile_ = profile;
    target_tab_url_ = target_tab_url;
    target_tab_title_ = target_tab_title;
    problem_type_ = problem_type;
    page_url_ = page_url;
    description_ = description;
    image_ = image;
#if defined(OS_CHROMEOS)
    user_email_ = user_email;
    send_sys_info_ = send_sys_info;
    sent_report_ = sent_report;
#endif
  }

#if defined(OS_CHROMEOS)
  void SyslogsComplete(chromeos::LogDictionaryType* logs,
                       std::string* zip_content);
#endif

  const std::string& target_tab_url() { return target_tab_url_; }
  const string16& target_tab_title() { return target_tab_title_; }

  int problem_type() { return problem_type_; }
  const std::string& page_url() { return page_url_; }
  const std::string& description() { return description_; }
  const std::vector<unsigned char>& image() { return image_; }
#if defined(OS_CHROMEOS)
  const std::string& user_email() { return user_email_; }
  const chromeos::LogDictionaryType* sys_info() { return sys_info_; }
  const bool send_sys_info() { return send_sys_info_; }
  const bool sent_report() { return sent_report_; }
  const std::string* zip_content() { return zip_content_; }
#endif


 private:
  Profile* profile_;

  // Target tab url.
  std::string target_tab_url_;
  // Target tab page title.
  string16 target_tab_title_;

  int problem_type_;
  std::string page_url_;
  std::string description_;
  std::vector<unsigned char> image_;

#if defined(OS_CHROMEOS)
  // Chromeos specific values for SendReport.
  std::string user_email_;
  chromeos::LogDictionaryType* sys_info_;
  // Content of the compressed system logs.
  std::string* zip_content_;
  // NOTE: Extra boolean sent_report_ is required because callback may
  // occur before or after we call SendReport().
  bool sent_report_;
  // Flag to indicate to SyslogsComplete that it should send the report
  bool send_sys_info_;
#endif
};

#endif  // CHROME_BROWSER_BUG_REPORT_DATA_H_
