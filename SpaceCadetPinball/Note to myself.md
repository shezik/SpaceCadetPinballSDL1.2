
When rotozoom returns a new surface it'll **always** be in RGBA32 format.

1. Create new surface. Size is determined by srcrect, need not to tackle its alpha-ness.
2. Blit from source surface (rect) to new surface.
3. Use SDL_gfx to stretch new surface to size of dstrect. This also creates a new surface.
4. Blit new-new surface to destination surface. Blend mode should be set by user before calling SDL_RenderCopy.

Stretch source surface first to optimize one step?
1. Use SDL_gfx to stretch source surface to match dstrect's dimensions, involving srcrect and math and stuff. This creates a new surface.
2. Calculate what the new srcrect should be, then use it to blit to destination surface.

Is streching a big surface first faster or is creating multiple surfaces faster?  
Implement both then profile. (I kinda think the latter one's faster...?)
