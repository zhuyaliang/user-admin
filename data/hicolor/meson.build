actions_icons = [
  ['16x16', 'user-admin.png'],
  ['22x22', 'user-admin.png'],
  ['24x24', 'user-admin.png'],
  ['32x32', 'user-admin.png'],
  ['48x48', 'user-admin.png']
]

foreach icon: actions_icons
  install_data(
    join_paths(icon[0], 'apps', icon[1]),
    install_dir: join_paths(get_option('prefix'), 'share/icons', 'hicolor', icon[0], 'apps')
  )
endforeach
