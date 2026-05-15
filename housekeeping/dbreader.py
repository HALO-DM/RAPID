from sql import SQL

class DBReader:

    def __init__(self, sql, channels):
        self.sql = sql
        self.channels = channels

        self.scids = {}
        for ch in channels:
            scid = sql.getSCID(ch)
            if scid < 0:
                raise RuntimeError(f"Invalid SCID for channel: {ch}")
            self.scids[ch] = scid

        last = sql.lastUpdate()
        self.last_timestamp = int(last.timestamp()) if last else 0

    def read_latest(self):
        timestamps = self.sql.getSCTimes(self.last_timestamp)

        if not timestamps:
            return None

        ts = max(timestamps)

        rows = self.sql.getSCValues(list(self.scids.values()), ts)

        if not rows:
            return None

        record = rows[0]

        result = {
            "time": int(record["time"].timestamp()),
            "values": {}
        }

        for i, ch in enumerate(self.channels):
            raw = record.get(f"value-{i+1}")
            try:
                val = float(raw)
                if val > -9:
                    result["values"][ch] = val
            except:
                pass

        self.last_timestamp = ts

        return result
