// simple script for testing `syncthingctl edit --script …` manually
config.folders.filter(f => f.id.startsWith("docs-")).forEach(f => f.paused = !f.paused)
