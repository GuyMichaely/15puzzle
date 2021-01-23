'''
used to brute force the special cases of the last 2 cells in a column
not in their correct position and in the top right 2x2 of the unsolved board
'''

# for the case where 0 is in last col
def bounds1(moves, zero):
  if -3 in moves and zero < 3:
    moves.remove(-3)
  if 3 in moves and zero > 2:
    moves.remove(3)
  if 1 in moves and (zero - 2) % 3 == 0:
    moves.remove(1)
  if -1 in moves and zero % 3 == 0:
    moves.remove(-1)

# for the case where 0 is in the second to last cell
def bounds2(moves, zero):
  if -3 in moves and zero < 3:
    moves.remove(-3)
  if -1 in moves and zero % 3 == 0:
    moves.remove(-1)
  if 1 in moves and ((zero - 2) % 3 == 0 or zero == 7):
    moves.remove(1)
  if 3 in moves and zero >= 5:
    moves.remove(3)

def getMoves(a, b, bounds, zero, movesLeft=18, moves=[-1, -3], moveHistory=()):
  # done condition
  if a == 2 and b == 5:
    return (True, movesLeft, moveHistory)
  
  if movesLeft:
    bounds(moves, zero) # dissallow illegal moves
    efficient = (False, 0, None) # potential solution
    for move in moves:
      zero += move
      if zero == a:
        a -= move
      elif zero == b:
        b -= move
      newMoveHistory = moveHistory + (move,)
      newMoves = [1, -1, 3, -3]
      newMoves.remove(-move) # don't move forth then immedietly back
      candidate = getMoves(a, b, bounds, zero, movesLeft - efficient[1] - 1, newMoves, newMoveHistory)
      if candidate[0] and candidate[1] >= efficient[1]: # solution exists and has fewer moves
        efficient = candidate
      # reset for next move
      zero -= move
      if a == zero:
        a += move
      elif b == zero:
        b += move
    return efficient
  return (False,)

def mapToChar(i):
  if i == -1:
    return 'l'
  if i == 1:
    return 'r'
  if i == -3:
    return 'u'
  if i == 3:
    return 'd'

print("0 in last col")
for a in (1, 2, 4):
  for b in (1, 2, 4):
    if a == b:
      continue
    result = getMoves(a, b, bounds1, 5)
    print(f'({a // 3}, {a %3 - 1}), ({b // 3}, {b % 3 - 1}): "{"".join(mapToChar(move) for move in result[2])}"')

print("0 in 2nd to last col")
for a in (1, 2, 4, 5):
  for b in (1, 2, 4, 5):
    if a == b:
      continue
    result = getMoves(a, b, bounds2, 7)
    print(f'({a // 3}, {a %3 - 1}), ({b // 3}, {b % 3 - 1}): "{"".join(mapToChar(move) for move in result[2])}"')