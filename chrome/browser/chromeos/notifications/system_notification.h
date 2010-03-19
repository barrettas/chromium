// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_NOTIFICATIONS_SYSTEM_NOTIFICATION_H_
#define CHROME_BROWSER_CHROMEOS_NOTIFICATIONS_SYSTEM_NOTIFICATION_H_

#include <string>

#include "base/basictypes.h"
#include "base/move.h"
#include "base/ref_counted.h"
#include "base/string16.h"
#include "chrome/browser/chromeos/notifications/balloon_collection_impl.h"
#include "chrome/browser/notifications/notification_delegate.h"

class Profile;

namespace chromeos {

// The system notification object handles the display of a system notification

class SystemNotification {
 public:
  // The profile is the current user profile. The id is any string used
  // to uniquely identify this notification. The title is the title of
  // the message to be displayed. On creation, the message is hidden.
  SystemNotification(Profile* profile, std::string id, string16 title);

  ~SystemNotification();

  // Show will show or update the message for this notification
  void Show(const string16& message);

  // Hide will dismiss the notification, if the notification is already
  // hidden it does nothing
  void Hide();

  // Current visibility state for this notification
  bool visible() const { return visible_; }

 private:
  class Delegate : public NotificationDelegate {
   public:
    explicit Delegate(std::string id) : id_(base::move(id)) {}
    void Display() {}
    void Error() {}
    void Close(bool by_user) {}
    std::string id() const { return id_; }

   private:
    std::string id_;

    DISALLOW_COPY_AND_ASSIGN(Delegate);
  };

  Profile* profile_;
  BalloonCollectionImpl* collection_;
  scoped_refptr<Delegate> delegate_;
  string16 title_;
  bool visible_;

  DISALLOW_COPY_AND_ASSIGN(SystemNotification);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_NOTIFICATIONS_SYSTEM_NOTIFICATION_H_

