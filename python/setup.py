from distutils.core import setup, Extension
import commands

def pkgconfig(*packages, **kw):
    flag_map = {'-I': 'include_dirs', '-L': 'library_dirs', '-l': 'libraries'}
    for token in commands.getoutput("pkg-config --libs --cflags %s" % ' '.join(packages)).split():
        kw.setdefault(flag_map.get(token[:2]), []).append(token[2:])
    return kw

module = Extension('_sphinxbase',
                   sources=['_sphinxbasemodule.c'],
                   **pkgconfig('sphinxbase',
                               include_dirs=['../include'],
                               libraries=['sphinxbase']))

setup(name = 'SphinxBase',
      version = '0.1',
      description = 'Python interface to SphinxBase speech recognition utilities',
      packages = ['sphinxbase'],
      ext_modules = [module],
     ) 
