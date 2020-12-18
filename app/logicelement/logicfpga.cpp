#include "logicfpga.h"

LogicFpga::LogicFpga( Fpga* fpga ) :
  LogicElement( fpga->inputSize(), fpga->inputSize() ), lastClk( false ), fpga(fpga) {
  setOutputValue( 0, false );
}

void LogicFpga::_updateLogic( const std::vector< bool > &inputs ) {
  setOutputValue( 0, inputs[ 0 ] );
}
