from distutils.core import setup, Extension

module = Extension('sphinxbase._sphinxbase',
                   include_dirs = ['../include',
                                   '../i386-linux/include',
                                   # FIXME: Use pkg-config
                                   '/usr/local/include/sphinxbase/',
                                   ],
                   libraries = ['sphinxbase'],
                   sources = ['_sphinxbasemodule.c'])

setup(name = 'SphinxBase',
      version = '0.1',
      description = 'Python interface to SphinxBase speech recognition utilities',
      packages = ['sphinxbase'],
      ext_modules = [module],
     ) 
