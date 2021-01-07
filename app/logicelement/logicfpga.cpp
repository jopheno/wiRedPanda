#include "logicfpga.h"

LogicFpga::LogicFpga( Fpga* fpga ) :
  LogicElement( fpga->inputSize(), fpga->outputSize() ), lastClk( false ), fpga(fpga) {
  //setOutputValue( 0, false );
}

void LogicFpga::_updateLogic( const std::vector< bool > &inputs ) {

  for (size_t i = 0; i < inputs.size(); i++) {
      std::map<uint32_t, bool>::const_iterator it = fpga->getInputs().find(fpga->input(i)->getRemoteId());
      if (it != fpga->getInputs().end()) {
        fpga->setInput(it->first, inputs[i]);
      }
  }

  for (size_t i = 0; i < getOutputAmount(); i++) {
    std::map<uint32_t, bool>::const_iterator it = fpga->getOutputs().find(fpga->output(i)->getRemoteId());
    if (it != fpga->getOutputs().end()) {
      setOutputValue( i, it->second );
    }
  }
}
