# yobd design
The design of yobd is intended to allow querying arbitrary OBD II PIDs while not
requiring the application te be aware of how OBD II or CAN actually work. It
also attempts to achieve an efficient, bitpacked representation of the OBD II
responses. While this representation is not as convenient as a self-describing
representation like JSON, it is much more efficient than self-describing data
for routine querying of PIDs, such as querying vehicle speed several times a
    second. The intent is that the application has to lookup the descriptors for
    the OBD II responses it gets exactly once, after which data can flow without
    overhead. From there, the application can also perform aggegation or other
    tasks that require understanding the meaning of the OBD II responses.

Below is a rough description of the intended flow and usage of yobd:

## Ask yobd for CAN requests
First, the application needs to know which PIDs it would like to query. For each
PID being queried, the application can ask yobd to provide the appropriate CAN
request data for that PID.

## Send off CAN requests
Next, the application sends off CAN requests using the CAN frames provided by
yobd. This is done in a system-specific way, whether through SocketCAN, a
character device, or some other mechanism. It could even be done by querying
another system and sending the response over the network. The point is that yobd
is agnostic of the mechanism for interacting with the physical CAN layer.  Since
the rest of the API operates over OBD II rather than CAN, it is also possible to
use yobd to parse OBD II over a different physical layer than CAN, so you could
have OBD II over TCP/IP or similar.

## Ask yobd to parse CAN responses
Once the application receives CAN responses, it asks yobd to parse them.  yobd
returns a bitpacked representation of the parsed OBD II response. For example,
for OBD II PID 0x0C (engine RPM), yobd would perform the calculation `(256*A +
    B) / 4` and put the result in a buffer as a 4-byte float. Although this has
    now doubled the data size from 2 bytes of bitpacked CAN to 4 bytes of OBD
    II, this result can be readily aggregated, filtered, and operated on in
    general. It is also still far more compact than it would be in a
    self-describing format, such as JSON.

## Ask yobd for an OBD II response descriptor
Given that yobd yields data in a bitpacked format, the application needs a way
to unpack the data and describe it at higher level. Importantly, this need be
done only once per PID, rather than for each CAN response. The application
queries yobd for the data descriptor mapping to a given PID. yobd returns
descriptive data such as the PID's friendly name ("engine RPM"), its units
(RPM), its bitpacked response size (4 bytes), and its type (float). This
information allows the application to aggregate, filter, and perform other
operations on the data, as well as allowing it to pass the data on in a
self-describing format, such as JSON. One can picture a generic data handling
framework such as gstreamer efficiently querying and handling the PIDs while
passing self-describing summaries of this data to a higher-level component such
as javascript. This would allow javascript to subscribe to messages, getting a
JSON digest of events every few seconds, all while not overloading the system
with constant JSON serialization and deserialization.
