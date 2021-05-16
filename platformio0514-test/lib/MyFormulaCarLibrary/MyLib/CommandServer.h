#ifndef CommandServer_h
#define CommandServer_h

#include "AsyncIO.h"
#include "Common.h"
#include "LineCommand.h"

class CommandServer : public BaseSocketServer {
 public:
  // todo: move cmd to resource, to increase flexibility
  CommandProcessor cmd;

  CommandServer(int port) : BaseSocketServer(port), cmd(resource->_buf_rx) {
    resource->use_tx = true;
    resource->use_rx = true;
    resource->rx_notify_only = true;
    onData([this](BaseAsyncResource*, uint8_t* data, size_t len) {
      assert(len == 0);  // should in notify only mode
      cmd.checkNewLine();
    });
    cmd.onResponse([](String s) {
      logc(s + "\r\n");
    });
  }
  NONCOPYABLE(CommandServer);

  void use(LineHandler r) {
    cmd.use(r);
  }
};

#endif  // CommandServer_h
