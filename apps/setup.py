from distutils.core import setup

#
# aufrufen mit 'python setup.py install --prefix=$prefix'
#

setup(	name='flux',
	version='0.1',
	description='FluxML Python routines',
	author='13CFLUX2 devs',
	author_email='info@13cflux.net',
	url='https://www.13cflux.net',
	py_modules=['flux'],
	scripts=['fmlstats',
		'setparameters',
		'setmeasurements',
		'ftbl2fml','sbml2fml','fml2sbml',
		'fmlupdate', 'multiply_fml',
                'setinputs']
	)

