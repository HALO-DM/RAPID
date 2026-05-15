import midas.client
import threading
import queue


class MidasDAQReader(threading.Thread):

    def __init__(self, out_queue, prescale=1):
        super().__init__(daemon=True)

        self.queue = out_queue
        self.prescale = prescale
        self.counter = 0

        self.client = midas.client.MidasClient("daq-reader")
        self.buffer = self.client.open_event_buffer("SYSTEM")
        self.request = self.client.register_event_request(
            self.buffer, event_id=1
        )

        # load metadata once
        self.stage_meta = self.load_metadata()

    def run(self):

        while True:

            event = self.client.receive_event(self.buffer, async_flag=True)

            if event is None:
                self.client.communicate(10)
                continue

            bank_names = [b.name for b in event.banks.values()]

            # skip cryo events
            if "CRYO" in bank_names:
                continue

            # prescale
            self.counter += 1
            if self.counter % self.prescale != 0:
                continue

            parsed = self.parse_event(event)

            if parsed:
                try:
                    self.queue.put_nowait(parsed)
                except queue.Full:
                    pass

    def parse_event(self, event):

        groups = {}

        for name, bank in event.banks.items():

            name = name.strip()

            # Expect: Si00, Sq00, Ts00
            if len(name) < 4:
                continue

            prefix = name[:2]   # Si, Sq, Ts
            idx_str = name[2:]  # "00", "01", ...

            typ_map = {
                "Si": "i",
                "Sq": "q",
                "Ts": "t"
            }

            if prefix not in typ_map:
                continue

            typ = typ_map[prefix]

            if idx_str not in groups:
                groups[idx_str] = {}

            data = bank.data

            # Handle scalar Ts
            # Handle Ts (always length-1 array)
            if typ == "t":

                # Extract timestamp (first element of TsXX)
                if hasattr(data, "__len__") and len(data) > 0:
                    ts_val = float(data[0])
                else:
                    ts_val = float(data)

                # Store real timestamp separately
                groups[idx_str]["ts"] = ts_val

                # Do NOT use Ts as x-axis
                groups[idx_str]["t"] = None

            else:
                groups[idx_str][typ] = [float(x) for x in data]

        # Fix missing time axis
        for idx_str, g in groups.items():

            if g.get("t") is None:
                if "i" in g:
                    g["t"] = list(range(len(g["i"])))
                elif "q" in g:
                    g["t"] = list(range(len(g["q"])))
                else:
                    g["t"] = []

        # for k, v in groups.items():
        #     print("STAGE", k, "->", list(v.keys()))

        return {
            "timestamp": event.header.timestamp,
            "waveforms": groups,
            "meta": self.stage_meta
        }

    def load_metadata(self):

        meta = {}

        try:
            odb = self.client.odb_get("/Equipment/DAQ DSP/Settings/")

            for key, val in odb.items():

                # keep keys consistent with bank naming ("00", "01", ...)
                idx_str = str(key).zfill(2)

                meta[idx_str] = {
                    "type": val.get("type", "waveform"),
                    "x_unit": val.get("x_unit", "sample"),
                    "y_unit": val.get("y_unit", "arb"),
                    "sample_rate": val.get("sample_rate", 1),
                    "title": val.get("title", f"Stage {idx_str}")
                }
                print(meta)

        except Exception as e:
            print("ODB metadata load failed:", e)

        return meta
