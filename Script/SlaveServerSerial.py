#!/usr/bin/env python

# --------------------------------------------------------------------------- #
# import the modbus libraries we need
# --------------------------------------------------------------------------- #
#from pymodbus.version import version
#from pymodbus.server.asynchronous import StartTcpServer
import asyncio
from pymodbus.server import StartSerialServer
from pymodbus.server import StartAsyncSerialServer
from pymodbus.server import ModbusSerialServer

from pymodbus.device import ModbusDeviceIdentification
from pymodbus.datastore import ModbusSequentialDataBlock
from pymodbus.datastore import ModbusSlaveContext, ModbusServerContext
#from pymodbus.transaction import ModbusRtuFramer, ModbusAsciiFramer
from pymodbus.framer import FramerType
from IPython.display import clear_output
import sys


# --------------------------------------------------------------------------- #
# import the twisted libraries we need
# --------------------------------------------------------------------------- #
from twisted.internet.task import LoopingCall
from twisted.internet import reactor
# --------------------------------------------------------------------------- #
# configure the service logging
# --------------------------------------------------------------------------- #
import logging
logging.basicConfig()
log = logging.getLogger()
log.setLevel(logging.ERROR)

# --------------------------------------------------------------------------- #
# define your callback process
# --------------------------------------------------------------------------- #


async def printing_context(a):
    #clear_output(wait=True)
    print("printing context")
    running = ['🕐', '🕑','🕒','🕓','🕔','🕕','🕖','🕗','🕘','🕙','🕚','🕛']
    i = 0
    while True:
        context = a[0]
        register = 3
        slave_id = 0x01
        address = 0x0
        values = context[slave_id].getValues(register, address, count=5)
        print(''.join(' ' + str(value) + ' ' for value in values) + 'server:' +  running[i] , end='\r')
        i = (i + 1) % len(running)

        #sys.stdout.flush()
        # print("printing context loop")
        await asyncio.sleep(0.5)
    


async def run_updating_server():
    # ----------------------------------------------------------------------- # 
    # initialize your data store
    # ----------------------------------------------------------------------- # 
    
    store = {0x01 : ModbusSlaveContext(
        di=ModbusSequentialDataBlock(0, [0]*100),
        co=ModbusSequentialDataBlock(0, [0]*100),
        hr=ModbusSequentialDataBlock(0, [0]*100),
        ir=ModbusSequentialDataBlock(0, [0]*100))}
    

    context = ModbusServerContext(slaves=store, single=False)
    
    # ----------------------------------------------------------------------- # 
    # initialize the server information
    # ----------------------------------------------------------------------- # 
    identity = ModbusDeviceIdentification()
    identity.VendorName = 'pymodbus'
    identity.ProductCode = 'PM'
    identity.VendorUrl = 'http://github.com/riptideio/pymodbus/'
    identity.ProductName = 'pymodbus Server'
    identity.ModelName = 'pymodbus Server'
    #identity.MajorMinorRevision = version.short()
    
    # ----------------------------------------------------------------------- # 
    # run the server you want
    # ----------------------------------------------------------------------- # 
    
    asyncio.create_task(printing_context(a=(context,)))
    print("Starting server")
    await StartAsyncSerialServer(context, identity=identity, framer=FramerType.RTU, port='COM6', baudrate=115200, timeout=1)
    

    
    

if __name__ == "__main__":
    try:
        asyncio.run(run_updating_server())
    except KeyboardInterrupt:
        print("\nServer stopped")
        #ServerStop()
        pass