// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/string_util.h"
#include "base/values.h"
#include "chrome/browser/dom_ui/tips_handler.h"
#include "chrome/browser/profile.h"
#include "chrome/browser/web_resource/web_resource_service.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/web_resource/web_resource_unpacker.h"
#include "chrome/common/url_constants.h"
#include "googleurl/src/gurl.h"

DOMMessageHandler* TipsHandler::Attach(DOMUI* dom_ui) {
  dom_ui_ = dom_ui;
  tips_cache_ = dom_ui_->GetProfile()->GetPrefs()->
      GetMutableDictionary(prefs::kNTPTipsCache);
  return DOMMessageHandler::Attach(dom_ui);
}

void TipsHandler::RegisterMessages() {
  dom_ui_->RegisterMessageCallback("getTips",
      NewCallback(this, &TipsHandler::HandleGetTips));
}

void TipsHandler::HandleGetTips(const Value* content) {
  // List containing the tips to be displayed.
  ListValue list_value;

  // Holds the web resource data found in the preferences cache.
  ListValue* wr_list;

  // These values hold the data for each web resource item.
  int current_tip_index;
  std::string current_tip;

  if (tips_cache_ != NULL && tips_cache_->GetSize() >= 0) {
    if (tips_cache_->GetInteger(
        WebResourceService::kCurrentTipPrefName, &current_tip_index) &&
        tips_cache_->GetList(
        WebResourceService::kTipCachePrefName, &wr_list)) {
      if (wr_list && wr_list->GetSize() > 0)
      if (wr_list->GetSize() <= static_cast<size_t>(current_tip_index))
        current_tip_index = 0;
      if (wr_list->GetString(current_tip_index, &current_tip)) {
        DictionaryValue* tip_dict = new DictionaryValue();
        tip_dict->SetString(L"tip_html_text", current_tip);
        list_value.Append(tip_dict);
        tips_cache_->SetInteger(WebResourceService::kCurrentTipPrefName,
                                current_tip_index + 1);
      }
    }
  }

  // Send list of web resource items back out to the DOM.
  dom_ui_->CallJavascriptFunction(L"tips", list_value);
}

// static
void TipsHandler::RegisterUserPrefs(PrefService* prefs) {
  prefs->RegisterDictionaryPref(prefs::kNTPTipsCache);
  prefs->RegisterStringPref(prefs::kNTPTipsServer,
                            WebResourceService::kDefaultResourceServer);
}

bool TipsHandler::IsValidURL(const std::wstring& url_string) {
  GURL url(WideToUTF8(url_string));
  return !url.is_empty() && (url.SchemeIs(chrome::kHttpScheme) ||
                             url.SchemeIs(chrome::kHttpsScheme));
}

