#!/bin/python3
import argparse
import sys

def main(args):
    parser = argparse.ArgumentParser(description='Generate a .txt file which can be used for table value initialization in t4p4s')
    parser.add_argument('outputFile', type=str,
                        help='path of generated file')
    parser.add_argument('numberOfEntries', type=int, default=1,
                        help='number of entries in the generated table')

    args = parser.parse_args(args[1:])
    generateTable(args.outputFile, args.numberOfEntries)

def generateTable(filename, count):
    with open(filename, 'w') as file:
        for i in range(count):
            file.write(f"{i} 0\n")


if __name__ == '__main__':
    main(sys.argv)