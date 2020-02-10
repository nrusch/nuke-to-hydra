m = nuke.menu('Nodes').addMenu('Hydra')

m.addCommand('Lights/HydraDomeLight', 'nuke.createNode("HydraDomeLight")')
m.addCommand('HydraRender', 'nuke.createNode("HydraRender")')
