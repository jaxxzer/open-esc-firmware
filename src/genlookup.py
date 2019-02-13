import math

resolution = 300
maxDuty = 10000

print("#include <inttypes.h>\nuint16_t lookup[] = {"),
for i in range(0,resolution):
  print("%d%s" % (int(maxDuty*(math.sin(2 * math.pi * i / resolution) + 1)/2), ","))
print("};")
