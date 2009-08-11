// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/first_run.h"

#import "base/scoped_nsobject.h"
#include "base/sys_string_conversions.h"
#import "chrome/app/breakpad_mac.h"
#import "chrome/browser/cocoa/first_run_dialog.h"
#import "chrome/browser/cocoa/import_progress_dialog.h"
#include "chrome/browser/importer/importer.h"
#include "chrome/browser/metrics/user_metrics.h"
#include "chrome/browser/shell_integration.h"
#include "chrome/installer/util/google_update_constants.h"
#include "chrome/installer/util/google_update_settings.h"

// static
bool FirstRun::IsChromeFirstRun() {
  // Use presence of kRegUsageStatsField key as an indicator of whether or not
  // this is the first run.
  // See chrome/browser/google_update_settings_mac.mm for details on why we use
  // the defualts dictionary here.
  NSUserDefaults* std_defaults = [NSUserDefaults standardUserDefaults];
  NSDictionary* defaults_dict = [std_defaults dictionaryRepresentation];
  NSString* collect_stats_key = base::SysWideToNSString(
      google_update::kRegUsageStatsField);

  bool not_in_dict = [defaults_dict objectForKey:collect_stats_key] == nil;
  return not_in_dict;
}

// Class that handles conducting the first run operation.
// FirstRunController deletes itself when the first run operation ends.
class FirstRunController : public ImportObserver {
 public:
  explicit FirstRunController();
  virtual ~FirstRunController() {}

  // Overridden methods from ImportObserver.
  virtual void ImportCanceled() {
    FirstRunDone();
  }
  virtual void ImportComplete() {
    FirstRunDone();
  }

  // Display first run UI, start the import and return when it's all over.
  bool DoFirstRun(Profile* profile, ProcessSingleton* process_singleton);

 private:
  // This method closes the first run window and quits the message loop so that
  // the Chrome startup can continue. This should be called when all the
  // first run tasks are done.
  void FirstRunDone();


  scoped_refptr<ImporterHost> importer_host_;

  DISALLOW_COPY_AND_ASSIGN(FirstRunController);
};


bool OpenFirstRunDialog(Profile* profile,
                        bool homepage_defined,
                        ProcessSingleton* process_singleton) {
// OpenFirstRunDialog is a no-op on non-branded builds.
  FirstRunController* controller = new FirstRunController;
  return controller->DoFirstRun(profile, process_singleton);
}

FirstRunController::FirstRunController()
    : importer_host_(new ImporterHost) {
}

void FirstRunController::FirstRunDone() {
  // Set preference to show first run bubble and welcome page.
  // TODO(jeremy): Implement
  // FirstRun::SetShowFirstRunBubblePref();
  // FirstRun::SetShowWelcomePagePref();

  delete this;
}

bool FirstRunController::DoFirstRun(Profile* profile,
    ProcessSingleton* process_singleton) {
  // This object is responsible for deleting itself, make sure that happens.
  scoped_ptr<FirstRunController> gc(this);

  // Breakpad should not be enabled on first run until the user has explicitly
  // opted-into stats.
  // TODO: The behavior we probably want here is to enable Breakpad on first run
  // but display a confirmation dialog before sending a crash report so we
  // respect a user's privacy while still getting any crashes that might happen
  // before this point.  Then remove the need for that dialog here.
  DCHECK(IsCrashReporterDisabled());

  scoped_nsobject<FirstRunDialogController> dialog(
      [[FirstRunDialogController alloc] init]);

  // Set list of browsers we know how to import.
  ssize_t profiles_count = importer_host_->GetAvailableProfileCount();

  // TODO(jeremy): Test on newly created account.
  // TODO(jeremy): Correctly handle case where no browsers to import from
  // are detected.
  NSMutableArray *browsers = [NSMutableArray arrayWithCapacity:profiles_count];
  for (int i = 0; i < profiles_count; ++i) {
    std::wstring profile = importer_host_->GetSourceProfileNameAt(i);
    [browsers addObject:base::SysWideToNSString(profile)];
  }
  [dialog.get() setBrowserImportList:browsers];

  // FirstRunDialogController will call exit if "Cancel" is clicked.
  [dialog.get() showWindow:nil];

  // If user clicked cancel, bail - browser_main will return if we haven't
  // turned off the first run flag when this function returns.
  if ([dialog.get() userDidCancel]) {
    return false;
  }

// Don't enable stats in Chromium.
#if defined(GOOGLE_CHROME_BUILD)
  BOOL stats_enabled = [dialog.get() statsEnabled];

  // Breakpad is normally enabled very early in the startup process,
  // however, on the first run it's off by default.  If the user opts-in to
  // stats, enable breakpad.
  if (stats_enabled) {
    InitCrashReporter();
    InitCrashProcessInfo();
  }

  GoogleUpdateSettings::SetCollectStatsConsent(stats_enabled);
#endif  // GOOGLE_CHROME_BUILD

  // If selected set as default browser.
  BOOL make_default_browser = [dialog.get() makeDefaultBrowser];
  if (make_default_browser) {
    bool success = ShellIntegration::SetAsDefaultBrowser();
    DCHECK(success);
  }

  // Import bookmarks.
  if ([dialog.get() importBookmarks]) {
    const ProfileInfo& source_profile = importer_host_->GetSourceProfileInfoAt(
        [dialog.get() browserImportSelectedIndex]);
    int16 items = source_profile.services_supported;
    // TODO(port): Do the actual import in a new process like Windows.
    gc.release();
    StartImportingWithUI(nil, items, importer_host_.get(),
                         source_profile, profile, this, true);
  }

  return true;
}
