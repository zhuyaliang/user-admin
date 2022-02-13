gnome = import('gnome')

sources = [
'main.c',
'user-window.c',
'user-admin.c',
'user-base.c',
'user-face.c',
'user-info.c',
'user-list.c',
'user-password.c',
'user-share.c',
'user-crop.c',
'user-avatar.c',
'user-group.c',
'user-group-window.c',
'user-history.c',
'user-language.c',
]

executable('mate-user-admin',
  sources : sources,
  dependencies : [gtk_deps, pwquality_deps, act_deps, gas_deps, crypt_deps, md_deps,polkit_deps,fc_deps],
  include_directories: top_srcdir,
  install : true,
  install_dir : get_option('bindir')
  )
