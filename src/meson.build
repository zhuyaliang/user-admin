gnome = import('gnome')

sources = [
'main.c',
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
'user-history.c',
'user-language.c',
]

account_src = gnome.gdbus_codegen(
  'account-generated',
  'org.freedesktop.Accounts.xml',
  interface_prefix : 'org.freedesktop.',
  namespace : 'Admin'
)

account_user_src = gnome.gdbus_codegen(
  'account-user-generated',
  'org.freedesktop.Accounts.User.xml',
  interface_prefix : 'org.freedesktop.',
  namespace : 'Admin'
  )

executable('mate-user-admin',
  sources : sources,
  dependencies : [gtk_deps, pwquality_deps, act_deps, gas_deps, crypt_deps, md_deps,polkit_deps,fc_deps],
  include_directories: top_srcdir,
  install : true,
  install_dir : get_option('bindir')
  )
