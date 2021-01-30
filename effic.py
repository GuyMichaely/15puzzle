'''
use brute force to solve the special case of the last 2 cells in a column
'''

def lettersToTuple(letters):
  m = {
    'u': -3,
    'r': 1,
    'd': 3,
    'l': -1
  }
  return tuple(m[l] for l in letters)

# 3x3 in top right of board
def bounds3(moves, zero):
  if -3 in moves and zero < 3:
    moves.remove(-3)
  if -1 in moves and zero % 3 == 0:
    moves.remove(-1)
  if 1 in moves and ((zero - 2) % 3 == 0 or zero == 7):
    moves.remove(1)
  if 3 in moves and zero > 4:
    moves.remove(3)

# manhattan distance
def d(a, b):
  return abs(a // 3 - b // 3) + abs(a % 3 - b % 3)

def getMoves(a, b, boundsFunc, zero, moves, maxMoves=19, moveHistory=()):
  # done condition
  if a == 2 and b == 5:
    return (True, maxMoves, moveHistory)
  
  originalMax = maxMoves
  best = (False, 0, ()) # best seen so far
  if d(a, 2) + d(b, 5) <= maxMoves:
    boundsFunc(moves, zero) # dissallow illegal moves
    for move in moves:
      zero += move
      if zero == a:
        a -= move
      elif zero == b:
        b -= move
      newMoveHistory = moveHistory + (move,)
      newMoves = [1, -1, 3, -3]
      newMoves.remove(-move) # don't move forth then immedietly back
      allowance = maxMoves - 1 # recursive case can use at most one less than us
      allowance -= best[0] # if we have found a good solution we want to find a better one
      # therefore remove one move from the recursive case, in which case
      # we know that if it returned true we found a shorter solution
      candidate = getMoves(a, b, boundsFunc, zero, newMoves, allowance, newMoveHistory)
      if candidate[0]: # solution exists and has fewer moves
        movesLeft = candidate[1] # candidate[1] is the unused moves
        # best starts out as (False, ...)
        # then is updated to (True, ...) and never goes back to (False, ...) 
        # if candidate replaces an old best, this call scope
        # gave it 2 less move than it has
        # otherwise, it gave it 1 less
        # this accounts for that
        replacesOld = best[0]
        movesLeft += replacesOld 
        best = (candidate[0], candidate[1] + replacesOld, candidate[2])
        
        maxMoves -= best[1]
      # reset for next move
      zero -= move
      if a == zero:
        a += move
      elif b == zero:
        b += move
    if best[0]:
      best = (best[0], originalMax - maxMoves, best[2])
  return best

# used for pretty printing
charmap = {
  -1: 'l',
  1: 'r',
  -3: 'u',
  3: 'd'
}

def isTrap(a, b):
  return a == 1 and b in [4, 5]

def isImpossible(a, b, z):
  return z == 2 and (isTrap(a, b) or isTrap(b, a))

def doConfig(log, a, b):
  # will hold move distances for an AB configuration
  data = [[None, None, None], [None, None, None], [None, None, None]]
  data[a // 3][a % 3] = 'a'
  data[b // 3][b % 3] = 'b'
  aDisplay = (a + 1 - a // 3) % 2 + 2 * (a // 3)
  bDisplay = (b + 1 - b // 3) % 2 + 2 * (b // 3)
  for z in range(8):
    if z == a or z == b:
      continue
    if isImpossible(a, b, z):
      continue
    result = getMoves(a, b, bounds3, z, [-3, -1, 1, 3])
    log[(aDisplay, bDisplay, z)] = ''.join(charmap[i] for i in result[2])
    data[z // 3][z % 3] = len(result[2])
  print(f'a={aDisplay} b={bDisplay}')
  print(
    '\n---------\n'.join(
      '|'.join(
        '  ' if e is None else '%2s' % str(e) for e in row
      ) for row in data
    ), 
    '\n'
  )

log = {} # store move lists
for a in [1, 2, 4, 5]:
  for b in [1, 2, 4, 5]:
    if a == b:
      continue
    if a == 2 and b == 5:
      continue
    doConfig(log, a, b)
for key in log:
  print(key, log[key])

log = {}
doConfig(log, 2, 3)
doConfig(log, 2, 6)
for key in log:
  print(key, log[key])