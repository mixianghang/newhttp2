#!/usr/bin/python
import sys
import os
import shutil
def main():
  if len(sys.argv) < 3:
    print "Usage sourceDir resultDir"
    sys.exit(1)
  sourceDir = sys.argv[1]
  resultDir = sys.argv[2]
  if not os.path.exists(sourceDir):
    print "{0} doesn't exist".format(sourceDir)
    sys.exit(1)
  if os.path.exists(resultDir):
    shutil.rmtree(resultDir + "_bak")
    shutil.move(resultDir, resultDir + "_bak")
    print "{0} exists, rename it".format(resultDir)
  os.makedirs(resultDir)
  
  for sourceFileName in os.listdir(sourceDir):
    sourceFilePath = os.path.join(sourceDir, sourceFileName)
    resultFilePath = os.path.join(resultDir, sourceFileName)
    resultFd = open(resultFilePath, "w")
    resultFd.write("sizeWhenCancel in KB\tpacketNumWhenCancel\tsizeOfAll in KB\tpacketNumOfAll\trecvedAfterCancel in KB\n")
    with open(sourceFilePath, "r") as fd:
      for line in fd:
        line = line[:-1]
        lineArray = line.split("\t")
        if len(lineArray) < 4:
          print "parse line {0} failed for file {1}".format(line, sourceFilePath)
          sys.exit(1)
        sizeWhenCancel = int(lineArray[0])
        sizeOfAll      = int(lineArray[2])
        lineArray.append(sizeOfAll - sizeWhenCancel)
        resultFd.write("\t".join(str(item) for item in lineArray) + "\n")
    resultFd.close()

if __name__ == "__main__":
  main()

  
