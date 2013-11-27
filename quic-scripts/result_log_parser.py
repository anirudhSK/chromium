filename = "/home/somakrdas/src/result.log"

filelines = open(filename,"r").read().splitlines();

counter = -1

for fileline in filelines:
  counter += 1
  if counter % 2 != 0:
    continue

  if "real\t" in fileline:
    _, time = fileline.split("\t")
    minutes, seconds = time.split('m')
    seconds, _ = seconds.split('s')
    minutes = float(minutes)
    seconds = float(seconds)
    print (60 * minutes) + (seconds)

    #numbers = [token for token in fileline.split() if token.isdigit()]
    #print numbers[0], "\t", numbers[1]

#  if "Bytes in this tick = " in fileline:
#    numbers = [token for token in fileline.split() if token.isdigit()]
#    number = int(numbers[0])
#    m_total_bytes += number
