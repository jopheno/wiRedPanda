#ifndef LOGICFPGA_H
#define LOGICFPGA_H

#include "logicelement.h"
#include "fpga.h"

class LogicFpga : public LogicElement {
public:
  explicit LogicFpga( Fpga* fpga );

  /* LogicElement interface */
protected:
  virtual void _updateLogic( const std::vector< bool > &inputs );

private:
  bool lastClk;
  bool lastValue;
  Fpga* fpga;
};


#endif // LOGICFPGA_H
