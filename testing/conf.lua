function love.conf(t)
  t.console = true
  t.window.name = 'love.test'
  t.window.width = 360
  t.window.height = 240
  t.window.resizable = true
  t.window.depth = true
  t.window.stencil = true
  t.renderers = {"opengl"}
end
