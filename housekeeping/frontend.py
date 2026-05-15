import midas
import midas.frontend
import midas.event
from equipment import CryoPeriodicEquipment

from sql import SQL
from dbreader import DBReader

class CryoFrontend(midas.frontend.FrontendBase):
    def __init__(self, dbreader):
        # must call __init__ from the base class.
        midas.frontend.FrontendBase.__init__(self, "HouseKeeping")
        
        # Can add equipment at any time before the call `run()`, but doing
        # it in __init__() seems logical.
        self.add_equipment(CryoPeriodicEquipment(self.client, dbreader))
        
    def begin_of_run(self, run_number):
        self.set_all_equipment_status("Running", "greenLight")

        # Call mapping setup for each equipment
        for eq in self.equipment.values():
            if hasattr(eq, "write_odb_mapping"):
                eq.write_odb_mapping()

        self.client.msg("Frontend has seen start of run number %d" % run_number)

        return midas.status_codes["SUCCESS"]
        
    def end_of_run(self, run_number):
        self.set_all_equipment_status("Finished", "greenLight")
        self.client.msg("Frontend has seen end of run number %d" % run_number)
        return midas.status_codes["SUCCESS"]
    
    def frontend_exit(self):
        for eq in self.equipment.values():
            if hasattr(eq, "dbreader"):
                eq.dbreader.sql.close()

        print("Goodbye from user code!")
