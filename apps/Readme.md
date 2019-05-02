
## Tools

All tools support the `-h`option, which will list a short description of the tool, and all available options.

### fmllint

fmllint checks if a file conforms to the fml syntax. To execute it simply run:

`fmllint -i file.fml`. 

### fmlupdate

Update the FluxML Level of an fml file. To update a file from level 1 to 2 run:

`fmlupdate -i level1_file.fml -o level2_file.fml -l 2`

### ftbl2fml

Converts a file from the old tabular ftbl format to fml. There are many options availble but the basic usage is:

`ftbl2fml -i ftbl_file.csv -o fml_file.fml`

### sbml2fml fml2sbml

Converts sbml files to fml files and vice versa. Note that the atom mapping is lost, wehn converting an fml file to an sbml file!

`sbml2fml -i model.xml -o model.fml`
`fml2sbml -i model.fml -o model.yml`

### fmlstats

Prints some statistical information about the network, ie number of:
* metabolites
* reactions
* atoms
* isotopomers
* cumomers (in unreduced network)
* cumomers per level of the cascade

The call is simply `fmlstats -i file.fml`

### multiply_fml

Multiplies a network once per configuration.

`multiply_fml -i model_with_n_configurations.fml -o model_with_n_fold_network.fml`

### setinputs setparameters setmeasurements

tools to set inputs (i.e. substrates) parameters (i.e. free fluxes and poolsizes) and measurements to a fml file. Their syntax is identical:

`setXXX -i original.fml -C data.csv -o modified.fml`


