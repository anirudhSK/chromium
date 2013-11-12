filename = "/tmp/server.log"

filelines = open(filename,"r").read().splitlines();

m_total_bytes = 0

for fileline in filelines:
#  if "Current throughput = " in fileline:
#    numbers = [token for token in fileline.split() if token.isdigit()]
#    number = int(numbers[0])
#    print number

  if "Send delta = " in fileline:
    numbers = [token for token in fileline.split() if token.isdigit()]
    print numbers[0], "\t", numbers[1]

#  if "Bytes in this tick = " in fileline:
#    numbers = [token for token in fileline.split() if token.isdigit()]
#    number = int(numbers[0])
#    m_total_bytes += number

# print "---------"
# print m_total_bytes
