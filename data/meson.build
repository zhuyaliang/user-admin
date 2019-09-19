subdir('face')
subdir('icon')
subdir('hicolor')
subdir('mate-user-admin')

i18n = import('i18n')

i18n.merge_file(
  'mate-user-admin-desktop',
  input: 'mate-user-admin.desktop.in',
  output: 'mate-user-admin.desktop',
  install: true,
  po_dir: join_paths(meson.source_root(), 'po'),
  install_dir : join_paths(get_option('prefix'), 'share/applications'),
  type: 'desktop',
)
polkit = 'org.mate.user.admin.policy'
i18n.merge_file(
  polkit,
  input: polkit + '.in',
  output: polkit,
  po_dir: join_paths(meson.source_root(), 'po'),
  install: true,
  install_dir: join_paths(get_option('prefix'), 'share/polkit-1', 'actions')
)  

appdata = 'mate-user-admin.appdata.xml'
i18n.merge_file(
  polkit,
  input: appdata + '.in',
  output: appdata,
  po_dir: join_paths(meson.source_root(), 'po'),
  install: true,
  install_dir: join_paths(get_option('prefix'), 'share/metainfo')
)  
