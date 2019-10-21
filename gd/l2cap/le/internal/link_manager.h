/*
 * Copyright 2019 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <memory>
#include <unordered_map>
#include <utility>

#include "os/handler.h"

#include "l2cap/internal/parameter_provider.h"
#include "l2cap/internal/scheduler.h"
#include "l2cap/le/fixed_channel_manager.h"
#include "l2cap/le/internal/fixed_channel_service_manager_impl.h"
#include "l2cap/le/internal/link.h"

#include "hci/acl_manager.h"
#include "hci/address.h"

namespace bluetooth {
namespace l2cap {
namespace le {
namespace internal {

class LinkManager : public hci::LeConnectionCallbacks {
 public:
  LinkManager(os::Handler* l2cap_handler, hci::AclManager* acl_manager, FixedChannelServiceManagerImpl* service_manager,
              l2cap::internal::ParameterProvider* parameter_provider)
      : l2cap_handler_(l2cap_handler), acl_manager_(acl_manager), service_manager_(service_manager),
        parameter_provider_(parameter_provider) {
    acl_manager_->RegisterLeCallbacks(this, l2cap_handler_);
  }

  struct PendingFixedChannelConnection {
    os::Handler* handler_;
    FixedChannelManager::OnConnectionFailureCallback on_fail_callback_;
  };

  struct PendingLink {
    std::vector<PendingFixedChannelConnection> pending_fixed_channel_connections_;
  };

  struct LinkIdentifier {
    hci::Address address;
    hci::AddressType address_type;
    LinkIdentifier(hci::Address address, hci::AddressType address_type)
        : address(address), address_type(address_type){};

    bool operator==(const LinkIdentifier& li) const {
      return address == li.address && address_type == li.address_type;
    }

    struct Hasher {
      std::size_t operator()(const LinkIdentifier& li) const {
        std::hash<bluetooth::hci::Address> h;
        return h(li.address);
      }
    };
  };

  // ACL methods

  Link* GetLink(hci::Address device, hci::AddressType address_type);
  void OnLeConnectSuccess(std::unique_ptr<hci::AclConnection> acl_connection) override;
  void OnLeConnectFail(hci::Address device, hci::AddressType address_type, hci::ErrorCode reason) override;
  void OnDisconnect(hci::Address device, hci::AddressType address_type, hci::ErrorCode status);

  // FixedChannelManager methods

  void ConnectFixedChannelServices(hci::Address device, hci::AddressType address_type,
                                   PendingFixedChannelConnection pending_fixed_channel_connection);

 private:
  // Dependencies
  os::Handler* l2cap_handler_;
  hci::AclManager* acl_manager_;
  FixedChannelServiceManagerImpl* service_manager_;
  l2cap::internal::ParameterProvider* parameter_provider_;

  // Internal states
  std::unordered_map<LinkIdentifier, PendingLink, LinkIdentifier::Hasher> pending_links_;
  std::unordered_map<LinkIdentifier, Link, LinkIdentifier::Hasher> links_;
  DISALLOW_COPY_AND_ASSIGN(LinkManager);
};

}  // namespace internal
}  // namespace le
}  // namespace l2cap
}  // namespace bluetooth