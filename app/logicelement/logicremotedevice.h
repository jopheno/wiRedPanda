#pragma once

#include "logicelement.h"
#include "remotedevice.h"

class LogicRemoteDevice : public LogicElement
{
public:
    explicit LogicRemoteDevice(RemoteDevice *RemoteDevice);

    void updateLogic() override;

    /* LogicElement interface */
protected:
    //void _updateLogic(const std::vector<bool> &inputs);

    Q_DISABLE_COPY(LogicRemoteDevice)

private:
    bool lastClk;
    bool lastValue;
    RemoteDevice *elm;
};
