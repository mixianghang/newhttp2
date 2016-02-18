#!/usr/bin/python
import os
import sys
import re

if len(sys.argv) < 4:
  print "Usage sourceDir resultDir resultName"
  sys.exit(1)

sourceDir = sys.argv[1]
resultDir = sys.argv[2]
resultName = sys.argv[3]

if not os.path.exists(sourceDir) or not os.path.isdir(sourceDir):
  print "SourceDir doesn't exist: {0}".format(sourceDir)

if not os.path.exists(resultDir):
  print "resultdir doesn't exist, so create it"
  os.makedirs(resultDir)

mergedFilePath = os.path.join(resultDir, "{0}_mergedData.csv".format(resultName))
statFilePath = os.path.join(resultDir, "{0}_statistic.csv".format(resultName))
if os.path.exists(mergedFilePath):
  print "result file exists, bak it"
  os.rename(mergedFilePath, mergedFilePath + "_bak")
if os.path.exists(statFilePath):
  print "result file exists, bak it"
  os.rename(statFilePath, statFilePath + "_bak")
mergedFd = open(mergedFilePath, "a")
statFd = open(statFilePath, "a")
mergedFd.write("type, operation, timeCostBeforeOperation(Second), recvedWhenOperation(KB), sniffedPacketNum, sniffedPayload(KB), recvedAfterOperation\n");
statFd.write("type, operation, timeCostBeforeOperation(Second), recvedWhenOperation(KB), sniffedPacketNum, sniffedPayload(KB), recvedAfterOperation, testNum\n");
sourceRe = re.compile(".*\.csv$", re.I)
fileNum = 0
for sub in os.listdir(sourceDir):
  if os.path.isdir(sub):
    continue
  if not sourceRe.match(sub):
    continue
  parts = sub.split("_")
  if len(parts) < 3:
    print "error to parse file {0}".format(sub)
  type = parts[1] + "_" + parts[2]
  operation = parts[0]
  fileNum += 1
  subFilePath = os.path.join(sourceDir, sub)
  timeSum = 0
  recved  = 0
  sniffed = 0
  packetNum = 0
  with open(subFilePath, "r") as fd:
    lineNum = 0
    itemNum = 0
    for line in fd:
      lineNum += 1 
      if lineNum == 1:
        continue
      line = line.strip(" \t\r\n")
      lineParts = line.split(",")
      if len(lineParts) < 4:
        print "parse line of file {0} failed: {1}".format(sub, line)
        continue
      mergedFd.write("{0}, {1}, {2},{3}\n".format(type, operation, line, int(lineParts[3]) - int(lineParts[1])))
      itemNum   += 1
      timeSum   += int(lineParts[0])
      recved    += int(lineParts[1])
      packetNum += int(lineParts[2])
      sniffed   += int(lineParts[3])
    if itemNum > 0:
      formatStr = "{0}, {1}, {2}, {3}, {4}, {5}, {6}, {7}\n"
      statFd.write(formatStr.format(type, operation, timeSum / itemNum, recved / itemNum, packetNum / \
	  itemNum, sniffed / itemNum, (sniffed - recved) / itemNum, itemNum))

print "finish merge {0} source files".format(fileNum)
mergedFd.close()
statFd.close()
