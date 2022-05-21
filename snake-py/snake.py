"""
MIT License

Copyright 2022 Jenna Fligor

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
"""

# sys used for sys.exit()
import sys

# randint used for random spawning
from random import randint

# pygame is the GUI library used in this project
import pygame


# adjust these values to change the game [default]
# =====
# number of columns in map [50]
GRID_SIZE_X = 50
# number of rows in map [50]
GRID_SIZE_Y = 50
# size of each tile in pixels [15]
TILE_SIZE = 15
# stating fps (speed) [3]
FPS_INITIAL = 3
# number by which the fps is increased on each fruit pickup [0.5]
FPS_INCREASE = 0.5
# when 'True' stops fruits from spawning on the edges [False]
EASY_MODE = False
 # list of colors that the snake will be
SNAKE_COLORS = [(255,255,255),(200,200,200)]
# color of the fruit
FRUIT_COLOR = (255,0,0)
# =====

# arrays start from 0, so that neeeds to be accounted for
GRID_SIZE_X_0 = GRID_SIZE_X - 1
GRID_SIZE_Y_0 = GRID_SIZE_Y - 1

class App:
    """this is the main class that make up the game"""

    def __init__(self):

        # creates the pygame display surface (the GUI window)
        self.displaySurface = None

        # creates size information for the window based on game valuse at the
        # top of this document
        self.size = self.weight, self.height = (GRID_SIZE_X * TILE_SIZE),\
                                               (GRID_SIZE_Y * TILE_SIZE)

        self._running = None
        self.snake = None
        self.facing = None
        self.willBeFacing = None
        self.snakelen = None
        self.fps = None
        self.fruit = None
        self.fontLarge = None
        self.fontSmall = None

    # this function converts a game grid posion and a color tuple into a square
    # on the display surface in the coresponding location
    def tile(self,color,pos):
        pygame.draw.rect(
            self.displaySurface,
            color,
            (
                (pos[0]*TILE_SIZE),
                (pos[1]*TILE_SIZE),
                TILE_SIZE,
                TILE_SIZE
            )
        )

    # this is the game init function, it sets up the game, and then waits for
    # the user to start the game
    def on_init(self):
        # initialize pygame and it's font subcomponent
        pygame.init()
        pygame.font.init()

        # creates the window with the size generated in the class init function
        self.displaySurface = pygame.display.set_mode(
            self.size,
            pygame.HWSURFACE | pygame.DOUBLEBUF
        )

        # sets the name of the window to "Snake"
        pygame.display.set_caption("Snake")

        # sets the game to running, if this is False the main game loop will not run
        self._running = True

        # generates two font objects that are called to make text
        self.fontLarge = pygame.font.Font(pygame.font.get_default_font(), 36)
        self.fontSmall = pygame.font.Font(pygame.font.get_default_font(), 20)

        # creates a simple start screen
        self.displaySurface.fill((100,255,120))
        waiting = self.draw_caption(
            'Snake!', (0,0,0),\
            'Press a key or click to play.', (0,0,0)
        )
        while waiting:
            pygame.time.Clock().tick(10)
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    pygameExit()
                elif event.type in [pygame.MOUSEBUTTONDOWN, pygame.KEYDOWN]:
                    waiting = False

        # calls reset which sets the game environment to a new game
        self.on_reset()

    # called with each event (user input)
    def on_event(self, event):
        # if the event is a quit signal, stop the main game loop, which will
        # move to cleanup
        if event.type == pygame.QUIT:
            self._running = False
        # if event is a key press:
        elif event.type == pygame.KEYDOWN:
            # if the event is an arrow key press change the direction the snake
            # will be facing on the next loop call
            if event.key == pygame.K_UP and self.facing != 2:
                self.willBeFacing = 0
            elif event.key == pygame.K_RIGHT and self.facing != 3:
                self.willBeFacing = 1
            elif event.key == pygame.K_DOWN and self.facing != 0:
                self.willBeFacing = 2
            elif event.key == pygame.K_LEFT and self.facing != 1:
                self.willBeFacing = 3
            # if the event is an escape key press then call the pause function
            elif event.key == pygame.K_ESCAPE:
                self.on_pause()

    def on_loop(self):

        # changes the direction that the snake is facing based on the user input
        # generated by the event function
        self.facing = self.willBeFacing

        # moves the snake forward one tile in the direction it is facing
        if self.facing == 0:
            self.snake.append((self.snake[-1][0],(self.snake[-1][1] - 1)))
        elif self.facing == 1:
            self.snake.append(((self.snake[-1][0] + 1),self.snake[-1][1]))
        elif self.facing == 2:
            self.snake.append((self.snake[-1][0],(self.snake[-1][1] + 1)))
        elif self.facing == 3:
            self.snake.append(((self.snake[-1][0] - 1),self.snake[-1][1]))

        # checks if the head of the snake is located at the same tile as the
        # fruit
        if self.snake[-1] == self.fruit:
            # if it is, increases the length of the snake, the game speed, and
            # spawns a new fruit
            self.snakelen += 1
            self.fps += FPS_INCREASE
            self.fruit = (
                (
                    randint(
                        (0 + int(EASY_MODE)),
                        (GRID_SIZE_X_0 - int(EASY_MODE))
                    )
                ),
                (
                    randint(
                        (0 + int(EASY_MODE)),
                        (GRID_SIZE_Y_0 - int(EASY_MODE))
                    )
                )
            )

        # removes the tiles from the tail of the snake until the snake is the
        # length it should be
        while len(self.snake) > self.snakelen:
            self.snake.pop(0)

        # checks if the snake ran into the edge, if so calls the death function
        if self.snake[-1][0] < 0 or self.snake[-1][1] < 0 or\
           self.snake[-1][0] > GRID_SIZE_X_0 or self.snake[-1][1] > GRID_SIZE_Y_0:
            self.on_death()

        # checks if the snake ran into itself, if so calls the death function
        for bodytile in self.snake[:(self.snakelen - 1)]:
            if self.snake[-1] == bodytile:
                self.on_death()

    #renders the game
    def on_render(self):
        # fills the screen in to cover the last frame
        self.displaySurface.fill(0)
        # draws the fruit using the tile function
        self.tile(FRUIT_COLOR,self.fruit)
        # iterates over each tile in the snake and draws it with the tile function
        for colorIter, pos in enumerate(self.snake):
            self.tile(SNAKE_COLORS[(colorIter % len(SNAKE_COLORS))],pos)
        # updates the display surface with the changes
        pygame.display.update()

    def on_death(self):

        # creates a simple death screen with the players score
        self.displaySurface.fill((150,0,0))
        waiting = self.draw_caption(
            "You Died", (220,220,220),
            f"Your score was {(self.snakelen - 1)}, click to play again.",(200,200,200)
        )
        while waiting:
            pygame.time.Clock().tick(10)
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    pygameExit()
                elif event.type == pygame.MOUSEBUTTONDOWN:
                    waiting = False

        # calls reset to play again
        self.on_reset()

    def on_reset(self):
        # sets the fps (game speed) to the starting speed defined at the top of the program
        self.fps = float(FPS_INITIAL)
        # randomly selects an initial facing direction
        self.willBeFacing = randint(0,3)
        # randomly selects a starting point for the snake that isn at least 5
        # tiles away from the edge
        self.snake = [
            tuple(
                (
                    randint(5,(GRID_SIZE_X_0 - 5)),
                    randint(5,(GRID_SIZE_Y_0 - 5))
                )
            )
        ]
        # initialize snake length to 1
        self.snakelen = 1
        # spawns a starting fruit in a randow location
        self.fruit = (
                (
                    randint(
                        (0 + int(EASY_MODE)),
                        (GRID_SIZE_X_0 - int(EASY_MODE))
                    )
                ),
                (
                    randint(
                        (0 + int(EASY_MODE)),
                        (GRID_SIZE_Y_0 - int(EASY_MODE))
                    )
                )
            )

    def on_pause(self):

        # creates a pause screen
        pauseOverlay = pygame.Surface(self.size)
        pauseOverlay.set_alpha(128)
        pauseOverlay.fill((0,0,0))
        self.displaySurface.blit(pauseOverlay,(0,0))
        waiting = self.draw_caption(
            '- Paused -', (255,0,0),\
            'Press escape or click to resume.', (255,255,255)
        )
        while waiting:
            pygame.time.Clock().tick(10)
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    pygameExit()
                elif event.type == pygame.MOUSEBUTTONDOWN:
                    waiting = False
                elif event.type == pygame.KEYDOWN:
                    if event.key == pygame.K_ESCAPE:
                        waiting = False


    def draw_caption(self, title, title_color, subtitle, subtitle_color):
        textSurfaceTitle = self.fontLarge.render(title, True, title_color)
        textSurfaceSubtitle = self.fontSmall.render(subtitle, True, subtitle_color)

        textSurfaceTitleSize = textSurfaceTitle.get_size()
        textSurfaceSubtitleSize = textSurfaceSubtitle.get_size()

        self.displaySurface.blit(
            textSurfaceTitle,
            dest=(
                self.weight // 2 - textSurfaceTitleSize[0] // 2,
                self.height // 2 - textSurfaceTitleSize[1] - 10,
            ),
        )

        self.displaySurface.blit(
            textSurfaceSubtitle,
            dest=(
                self.weight // 2 - textSurfaceSubtitleSize[0] // 2,
                self.height // 2 + textSurfaceSubtitleSize[1] + 5,
            ),
        )

        pygame.display.update()
        return True

    # is called to start the app
    def on_execute(self):
        # calls the app init function, if this fails, closes the app
        if self.on_init() is False:
            self._running = False

        # main game loop
        while self._running:
            # limits loop to desired game speed, this increases as the player
            # collects fruit
            pygame.time.Clock().tick(self.fps)
            # gets all the events (user input) that have taken place since this
            # was last called and iterates through them
            for event in pygame.event.get():
                # calls the event handling function for each event
                self.on_event(event)
            # calls the loop function, calculates any/all changes to the game
            # from time passing and user input
            self.on_loop()
            # calls the render function, this updates the window with the changes
            self.on_render()

        # when the game loop is stopped, this calls the cleanup function
        pygameExit()

def pygameExit():
    pygame.quit()
    sys.exit()

if __name__ == "__main__" :
    theApp = App()
    theApp.on_execute()
