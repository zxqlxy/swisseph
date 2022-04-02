from os import replace
from subprocess import PIPE, run

julian_day = 2451247.48267
longtitude = 34.2658
latitude = 108.9401


command = ["./swetest", "-bj{}".format(julian_day), "-house{},{},W".format(longtitude, latitude)]
result = run(command, stdout=PIPE, stderr=PIPE, universal_newlines=True)
# print(result.returncode, result.stdout, result.stderr)

for i in result.stdout.split("\n"):
    # print(i)
    lis = i.split(" ")
    lis = list(filter(('').__ne__, lis))
    if "Sun" in lis:
        print("Sun : ", lis[1])
    if "Moon" in lis:
        print("Moon : ", lis[1])
    if "Venus" in lis:
        print("Venus : ", lis[1])
