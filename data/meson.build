i18n = import('i18n')

i18n.merge_file(
  'user-admin-desktop',
  input: 'user-admin.desktop.in',
  output: 'user-admin.desktop',
  install: true,
  po_dir: join_paths(meson.source_root(), 'po'),
  install_dir : join_paths(get_option('prefix'), 'share/applications'),
  type: 'desktop',
)

install_subdir('face',
  install_dir : join_paths(get_option('prefix'), 'share/user-admin'),
)
