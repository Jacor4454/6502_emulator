

if __name__ == "__main__":
     while(True):
          line = input()
          if len(line) == 4:
               line = line[0:3]
               tot = 0
               tot += ord(line[0]) - ord('A')
               tot *= 32
               tot += ord(line[1]) - ord('A')
               tot *= 32
               tot += ord(line[2]) - ord('A')
               # print(line, tot)
               print("case " + str(tot) + "://" + line + "\n    " + line + "();\n    break;")
          else :
               print("input of invalid size, try again")
