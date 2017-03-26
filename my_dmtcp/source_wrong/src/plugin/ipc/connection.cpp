/****************************************************************************
 *   Copyright (C) 2006-2013 by Jason Ansel, Kapil Arya, and Gene Cooperman *
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

#include <fcntl.h>
#include "connection.h"
#include "../jalib/jassert.h"
#include "../jalib/jserialize.h"

// sawada
#include <stdio.h>
#include <vector>
#include <algorithm>
extern std::vector<int> udsconnection; //復元しないソケットFD
extern std::vector<int> NotusingconnectionForOrted;
extern int blanceproc; //負荷分散プロセスがあるかどうかのフラグ
extern int ortedNewfd_is; //orted側で新しい受信用FDを用意したかどうかのフラグ
extern int orterunNewfd_is; //orterun側で新しい受信用FDを用意したかどうかのフラグ
//extern vector<int32_t> newfds;// 2016/2/23
//sawada 

dmtcp::Connection::Connection(uint32_t t)
  : _id(ConnectionIdentifier::Create())
  , _type((ConnectionType) t)
  , _fcntlFlags(-1)
  , _fcntlOwner(-1)
  , _fcntlSignal(-1)
  , _hasLock(false)
{}

void dmtcp::Connection::addFd(int fd)
{
  _fds.push_back(fd);
}

void dmtcp::Connection::removeFd(int fd)
{
  JASSERT(_fds.size() > 0);
  if (_fds.size() == 1) {
    JASSERT(_fds[0] == fd);
    _fds.clear();
  } else {
    for (size_t i = 0; i < _fds.size(); i++) {
      if (_fds[i] == fd) {
        _fds.erase(_fds.begin() + i);
        break;
      }
    }
  }
}

void dmtcp::Connection::saveOptions()
{
  
  errno = 0;
  _fcntlFlags = fcntl(_fds[0],F_GETFL);
  JTRACE("called saveOptions()")(_fcntlFlags)(_fds[0])(_type)(FD_CLOEXEC);
  JASSERT(_fcntlFlags >= 0) (_fds[0]) (_fcntlFlags) (_type) (JASSERT_ERRNO);
  errno = 0;
  _fcntlOwner = fcntl(_fds[0],F_GETOWN);
  JASSERT(_fcntlOwner != -1) (_fcntlOwner) (JASSERT_ERRNO);
  errno = 0;
  _fcntlSignal = fcntl(_fds[0],F_GETSIG);
  JASSERT(_fcntlSignal >= 0) (_fcntlSignal) (JASSERT_ERRNO);
}

void dmtcp::Connection::restoreOptions()
{
  // original :start
  // debug
    int y;
    for(y=0;y<udsconnection.size();y++){
      JTRACE("chek:udsconnection")(udsconnection[y])(y);
    }
  //

  //負荷分散プロセスが持つ「受信しなくなったfd」と、orterunがortedとの通信用に用意していたfdはrestoreしない
  if(blanceproc == 1 || orterunNewfd_is == 1 ){
      std::vector<int>::iterator f=std::find(udsconnection.begin(),udsconnection.end(),_fds[0]);
      if(f != udsconnection.end()){
        JTRACE("match,next is return")(_fds[0]);
        return;//何もせずこの関数を抜ける
      }
  }
  
  
  if(NotusingconnectionForOrted.empty()==false){
    std::vector<int>::iterator f=std::find(NotusingconnectionForOrted.begin(),NotusingconnectionForOrted.end(),_fds[0]);
      if(f != NotusingconnectionForOrted.end()){
        JTRACE("match,next is return")(_fds[0])(NotusingconnectionForOrted[0]);
	
        return;
      }
    }
  // original :end

  //restore F_GETFL flags
  JTRACE("called restoreOptions")(_fcntlFlags)(_fcntlOwner)(_fcntlSignal)(_fds[0]);
  JASSERT(_fcntlFlags >= 0) (_fcntlFlags);
  JASSERT(_fcntlOwner != -1) (_fcntlOwner);
  JASSERT(_fcntlSignal >= 0) (_fcntlSignal);
  errno = 0;

  /* Linux では、このコマンドで変更できるのは O_APPEND, O_ASYNC, O_DIRECT, O_NOATIME, O_NONBLOCK フラグだけ*/
  /*
  O_APPEND...ファイルを追加 (append) モードでオープンする.ファイルポインターをファイルの最後に移動.
  O_APPEND を使用すると、複数のプロセスがひとつのファイルに同時にデータを追加した場合、 ファイルが壊れてしまうことがある

  O_ASYNC...このファイルディスクリプターへの 入力または出力が可能になった場合に、シグナルを生成する
  この機能が使用可能なのは端末、疑似端末、ソケットのみであり、 (Linux 2.6 以降では) パイプと FIFO に対しても使用

  O_DIRECT...このファイルに対する I/O のキャッシュの効果を最小化しようとする。このフラグを使うと、一般的に性能が低下する。 しかしアプリケーションが独自にキャッシングを行っているような 特別な場合には役に立つ

  O_NONBLOCK...可能ならば、ファイルは非停止 (nonblocking) モードでオープンされる。 open() も、返したファイルディスクリプターに対する以後のすべての操作も呼び出 したプロセスを待たせることはない
*/
  JASSERT(fcntl(_fds[0], F_SETFL, (int)_fcntlFlags) == 0)
    (_fds[0]) (_fcntlFlags) (JASSERT_ERRNO);
  //JTRACE("After fcntl:F_SETFL")(_fds[0])(fcntl)(_fcntlFlags);

  errno = 0;

  /* ファイルディスクリプター fd のイベント発生を知らせるシグナル SIGIO や SIGURG を受けるプロセスの プロセス ID またはプロセスグループID を arg で指定された ID に設定
F_SETOWN により指定された所有者のプロセス (またはプロセスグループ) に シグナルを送る際には、 kill(2) に書かれているのと同じ許可のチェックが行われる。 このとき、シグナルを送信するプロセスは F_SETOWN を使ったプロセス

*/
  JASSERT(fcntl(_fds[0], F_SETOWN, (int)_fcntlOwner) == 0)
   (_fds[0]) (_fcntlOwner) (JASSERT_ERRNO);
  //JTRACE("After fcntl:F_SETOWN")(_fds[0])(fcntl)(_fcntlOwner);

  // This JASSERT will almost always trigger until we fix the above mentioned
  // bug.
  //JASSERT(fcntl(_fds[0], F_GETOWN) == _fcntlOwner)
  //(fcntl(_fds[0], F_GETOWN)) (_fcntlOwner) (VIRTUAL_TO_REAL_PID(_fcntlOwner));

  errno = 0;

  /*入力や出力が可能になった場合に送るシグナルを arg に指定された値に設定*/
  JASSERT(fcntl(_fds[0], F_SETSIG, (int)_fcntlSignal) == 0)
    (_fds[0]) (_fcntlSignal) (JASSERT_ERRNO);
  //JTRACE("After fcntl:F_SETSIG")(_fds[0])(fcntl)(_fcntlSignal);
}

void dmtcp::Connection::doLocking()
{
  errno = 0;
  _hasLock = false;
  JASSERT(fcntl(_fds[0], F_SETOWN, getpid()) == 0)
   (_fds[0]) (JASSERT_ERRNO);
}

void dmtcp::Connection::checkLock()
{
  
  pid_t pid = fcntl(_fds[0], F_GETOWN);
  JASSERT(pid != -1);
  _hasLock = pid == getpid();
  JTRACE("check _hasLock")(_hasLock);
}

void dmtcp::Connection::serialize(jalib::JBinarySerializer& o)
{
  JSERIALIZE_ASSERT_POINT("dmtcp::Connection");
  o & _id & _type & _fcntlFlags & _fcntlOwner & _fcntlSignal;
  o.serializeVector(_fds);
  serializeSubClass(o);
}
