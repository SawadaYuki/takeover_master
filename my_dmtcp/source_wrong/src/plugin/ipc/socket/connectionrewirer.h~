/****************************************************************************
 *   Copyright (C) 2006-2008 by Jason Ansel, Kapil Arya, and Gene Cooperman *
 *   jansel@csail.mit.edu, kapil@ccs.neu.edu, gene@ccs.neu.edu              *
 *                                                                          *
 *   This file is part of the dmtcp/src module of DMTCP (DMTCP:dmtcp/src).  *
 *                                                                          *
 *  DMTCP:dmtcp/src is free software: you can redistribute it and/or        *
 *  modify it under the terms of the GNU Lesser General Public License as   *
 *  published by the Free Software Foundation, either version 3 of the      *
 *  License, or (at your option) any later version.                         *
 *                                                                          *
 *  DMTCP:dmtcp/src is distributed in the hope that it will be useful,      *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 *  GNU Lesser General Public License for more details.                     *
 *                                                                          *
 *  You should have received a copy of the GNU Lesser General Public        *
 *  License along with DMTCP:dmtcp/src.  If not, see                        *
 *  <http://www.gnu.org/licenses/>.                                         *
 ****************************************************************************/

#pragma once
#ifndef CONNECTIONREWIRER_H
#define CONNECTIONREWIRER_H

#include <sys/socket.h>
#include <sys/un.h>
#include "dmtcpalloc.h"
#include "connectionidentifier.h"
#include "connection.h"


namespace dmtcp
{
  typedef map<ConnectionIdentifier, Connection*> ConnectionListT;

  class ConnectionRewirer
  {
    public:
      struct RemoteAddr {
        struct sockaddr_storage addr; //受信時にバイト数が不明の時に使う
        socklen_t len;
        Connection *con;
      };

      static ConnectionRewirer& instance();
      static void destroy();

      void openRestoreSocket(bool hasIPv4, bool hasIPv6, bool hasUNIX);
      void registerIncoming(const ConnectionIdentifier& local,
                            Connection *con,
                            int domain);
      void registerOutgoing(const ConnectionIdentifier& remote,
                            Connection *con);
      void registerNSData();
      
      void sendQueries();
      void doReconnect();
      void checkForPendingIncoming(int restoreSockFd, ConnectionListT *conList);

      // sawada org: start
      void printConList();
      void changeConnection(std::string *sProcLocation);
      void BalanceCheckForPendingIncoming(int restoreSockFd, ConnectionListT *conList);
      void checkForPendingIncomingForChangedCon(int restoreSockFd, ConnectionListT *conList,int size);
      //sawada org: end
     

      void debugPrint() const;
      
      //sawada org:start
      typedef map<ConnectionIdentifier,ConnectionIdentifier> ConInfo;
      void registerConInfo(const ConnectionIdentifier& local,const ConnectionIdentifier& remote,int domain);
      //sawada org:end

    private:
      void registerNSData(void *addr, socklen_t len, ConnectionListT *conList);

      struct sockaddr_in    _ip4RestoreAddr;
      socklen_t             _ip4RestoreAddrlen;
      struct sockaddr_in6   _ip6RestoreAddr;
      socklen_t             _ip6RestoreAddrlen;
      struct sockaddr_un    _udsRestoreAddr;
      socklen_t             _udsRestoreAddrlen;

      typedef ConnectionListT::iterator iterator;
      typedef ConnectionListT::const_iterator const_iterator;
      typedef map<ConnectionIdentifier, struct RemoteAddr> RemoteInfoT;
      typedef RemoteInfoT::iterator remoteInfoIter;

      ConnectionListT _pendingIP4Incoming;
      ConnectionListT _pendingIP6Incoming;
      ConnectionListT _pendingUDSIncoming;
      

      ConnectionListT _pendingOutgoing;
      RemoteInfoT     _remoteInfo;

      ConInfo _endpointConnectionInfo;
  };

}

//extern int udsconnection[5]={};
//add sawada 2015/8/4
//extern in_port_t parent_port;
//extern bool savedPortNum;

#endif


/*


struct sockaddr_storage {
        __uint8_t       ss_len;                  // address length 
        sa_family_t     ss_family;               // [XSI] address family 
        char            __ss_pad1[_SS_PAD1SIZE];
        __int64_t       __ss_align;              // force structure storage alignment 
        char            __ss_pad2[_SS_PAD2SIZE];
};
*/
