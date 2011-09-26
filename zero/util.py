def test_checksum (s):
  csum = s[-2:]
  i = 1
  l = len(s) - 3

  sum = 0
  while (i < l):
    sum = sum ^ ord(s[i])
    i += 1

  return (hex2(sum) == csum)

''' Version of hex aimed at byte values returning two digits 0 padded '''
def hex2(i):
  s = '0123456789ABCDEF'
  return s[i >> 4] + s[i & 0x0F]
