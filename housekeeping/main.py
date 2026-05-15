from frontend import CryoFrontend
from sql import SQL
from dbreader import DBReader

if __name__ == "__main__":

    options = ["localhost", "axion_reader", "8082", "axion_db"]

    sql = SQL(debug=False, options=options)

    # Cryo channels - change when appropriate
    channels = ["4switchA [K]","4pumpA [K]","3switchA [K]","3pumpA [K]","4switchB [K]","4pumpB [K]","3switchB [K]","3pumpB [K]","4HePotA [K]","3HePotA [K]","4HePotB [K]","3HePotB [K]","Condenser [K]","50K [K]","4K [K]","MC [K]","Still [K]","4switchAheat [V]","4switchBheat [V]","3switchAheat [V]","3switchBheat [V]","4pumpAheat [V]","4pumpBheat [V]","3pumpAheat [V]","3pumpBheat [V]","4HePotA [R]","4HePotB [R]","3HePotB [R]","3HePotA [R]","4pumpA [V]","4pumpB [V]","3pumpB [V]","3pumpA [V]","4switchA [V]","4switchB [V]","3switchB [V]","3switchA [V]","50K [V]","4K [V]","Condenser [R]"]

    dbreader = DBReader(sql, channels)

    with CryoFrontend(dbreader) as my_fe:
        my_fe.run()
