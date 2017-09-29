//This is a port of the bond-information in the residue profile file 
//fflevitt.rtp used in the original KGS. The following disclaimer was in 
//the head of fflevitt.rtp:
//; ########################################################################
//; #                                                                      #
//; #       ffamber ports for GROMACS 4.0.2 & 4.0.3 - February 2008        #
//; #       Eric J. Sorin - CSU Long Beach, Dept. of Chem & Biochem        #
//; #                  With significant contributions from                 #
//; #            Sanghyun Park, Jory Z. Ruscio, and Erik Thompson          #
//; #   Please cite: Sorin & Pande (2005). Biophys. J. 88(4), 2472-2493.   #
//; #            For additional information, tools, and updates            #
//; #          please consult http://chemistry.csulb.edu/ffamber/          #
//; #                                                                      #
//; ########################################################################

#include "Util.h" //CovBond

typedef std::map<std::string,std::vector<CovBond> > ResidueProfile;

//The COV_BOND_PROFILES specify which types of atoms in certain types of 
//residues have covalent bonds. A "-" in front of an atom-name indicates a 
//reference to the previous residue.
//
//The FIXED_BOND_PROFILES specify which covalent bonds should not permit
//rotation. Currently, all bonds specified here are partial double-bonds.
//Double bonds to atoms with only one single covalent neighbor are currently
//left out since they will be manually fixed by KGS anyway.

const char* const COV_BOND_PROFILES [][3] = {
    {"T3P","OW","HW1"},
    {"T3P","OW","HW2"},
    {"T3H","OW","HW1"},
    {"T3H","OW","HW2"},
    {"URE","C","N1"},
    {"URE","C","N2"},
    {"URE","C","O"},
    {"URE","C","OT"},
    {"URE","C","O1"},
    {"URE","C","O2"},
    {"URE","C","OT1"},
    {"URE","C","OT2"},
    {"URE","N1","H11"},
    {"URE","N1","H12"},
    {"URE","N2","H21"},
    {"URE","N2","H22"},
    {"ACE","HH31","CH3"},
    {"ACE","CH3","HH32"},
    {"ACE","CH3","HH33"},
    {"ACE","CH3","C"},
    {"ACE","C","O"},
    {"ACE","C","OT"},
    {"ACE","C","OT1"},
    {"ACE","C","OT2"},
    {"ACE","C","O1"},
    {"ACE","C","O2"},
    {"NME","N","H"},
    {"NME","N","HN"},
    {"NME","N","CH3"},
    {"NME","CH3","HH31"},
    {"NME","CH3","HH32"},
    {"NME","CH3","HH33"},
    {"NME","-C","N"},
    {"NHE","N","H1"},
    {"NHE","N","H2"},
    {"NHE","-C","N"},
    {"NH2","N","H1"},
    {"NH2","N","H2"},
    {"NH2","-C","N"},
    {"ALA","N","H"},
    {"ALA","N","HN"},
    {"ALA","N","H1"},
    {"ALA","N","H2"},
    {"ALA","N","H3"},
    {"ALA","N","HT1"},
    {"ALA","N","HT2"},
    {"ALA","N","HT3"},
    {"ALA","N","CA"},
    {"ALA","CA","HA"},
    {"ALA","CA","CB"},
    {"ALA","CA","C"},
    {"ALA","CB","HB1"},
    {"ALA","CB","HB2"},
    {"ALA","CB","HB3"},
    {"ALA","C","O"},
    {"ALA","C","OT"},
    {"ALA","C","OT1"},
    {"ALA","C","OT2"},
    {"ALA","C","OXT"},
    {"ALA","C","O1"},
    {"ALA","C","O2"},
    {"ALA","C","HXT"},
    {"ALA","-C","N"},
    {"GLY","N","H"},
    {"GLY","N","HN"},
    {"GLY","N","HT1"},
    {"GLY","N","HT2"},
    {"GLY","N","HT3"},
    {"GLY","N","H1"},
    {"GLY","N","H2"},
    {"GLY","N","H3"},
    {"GLY","N","CA"},
    {"GLY","CA","HA1"},
    {"GLY","CA","HA2"},
    {"GLY","CA","HA3"}, //added for 3msw
    {"GLY","CA","C"},
    {"GLY","C","O"},
    {"GLY","C","OT"},
    {"GLY","C","OT1"},
    {"GLY","C","OT2"},
    {"GLY","C","OXT"},
    {"GLY","C","O1"},
    {"GLY","C","O2"},
    {"GLY","C","HXT"},
    {"GLY","-C","N"},
    {"SER","N","H"},
    {"SER","N","HN"},
    {"SER","N","HT1"},
    {"SER","N","HT2"},
    {"SER","N","HT3"},
    {"SER","N","H1"},
    {"SER","N","H2"},
    {"SER","N","H3"},
    {"SER","N","CA"},
    {"SER","CA","HA"},
    {"SER","CA","CB"},
    {"SER","CA","C"},
    {"SER","CB","HB1"},
    {"SER","CB","HB2"},
    {"SER","CB","HB3"}, //added for 3msw
    {"SER","CB","OG"},
    {"SER","OG","HG"},
    {"SER","OG","HG1"},
    {"SER","C","O"},
    {"SER","C","OT"},
    {"SER","C","OT1"},
    {"SER","C","OT2"},
    {"SER","C","OXT"},
    {"SER","C","O1"},
    {"SER","C","O2"},
    {"SER","C","HXT"},
    {"SER","-C","N"},
    {"THR","N","HT1"},
    {"THR","N","HT2"},
    {"THR","N","HT3"},
    {"THR","N","H1"},
    {"THR","N","H2"},
    {"THR","N","H3"},
    {"THR","N","H"},
    {"THR","N","HN"},
    {"THR","N","CA"},
    {"THR","CA","HA"},
    {"THR","CA","CB"},
    {"THR","CA","C"},
    {"THR","CB","HB"},
    {"THR","CB","CG2"},
    {"THR","CB","OG1"},
    {"THR","CG2","HG21"},
    {"THR","CG2","HG22"},
    {"THR","CG2","HG23"},
    {"THR","OG1","HG1"},
    {"THR","C","O"},
    {"THR","C","OT"},
    {"THR","C","OT1"},
    {"THR","C","OT2"},
    {"THR","C","O1"},
    {"THR","C","O2"},
    {"THR","C","OXT"}, //added for 1ldl
    {"THR","C","HXT"},
    {"THR","-C","N"},
    {"LEU","N","H"},
    {"LEU","N","HN"},
    {"LEU","N","HT1"},
    {"LEU","N","HT2"},
    {"LEU","N","HT3"},
    {"LEU","N","H1"},
    {"LEU","N","H2"},
    {"LEU","N","H3"},
    {"LEU","N","CA"},
    {"LEU","CA","HA"},
    {"LEU","CA","CB"},
    {"LEU","CA","C"},
    {"LEU","CB","HB1"},
    {"LEU","CB","HB2"},
    {"LEU","CB","HB3"}, //added for 3msw
    {"LEU","CB","CG"},
    {"LEU","CG","HG"},
    {"LEU","CG","CD1"},
    {"LEU","CG","CD2"},
    {"LEU","CD1","HD11"},
    {"LEU","CD1","HD12"},
    {"LEU","CD1","HD13"},
    {"LEU","CD2","HD21"},
    {"LEU","CD2","HD22"},
    {"LEU","CD2","HD23"},
    {"LEU","C","O"},
    {"LEU","C","OT"},
    {"LEU","C","OT1"},
    {"LEU","C","OT2"},
    {"LEU","C","OXT"}, //added for 8atc
    {"LEU","C","O1"},
    {"LEU","C","O2"},
    {"LEU","C","HXT"},
    {"LEU","-C","N"},
    {"ILE","N","H"},
    {"ILE","N","HN"},
    {"ILE","N","HT1"},
    {"ILE","N","HT2"},
    {"ILE","N","HT3"},
    {"ILE","N","H1"},
    {"ILE","N","H2"},
    {"ILE","N","H3"},
    {"ILE","N","CA"},
    {"ILE","CA","HA"},
    {"ILE","CA","CB"},
    {"ILE","CA","C"},
    {"ILE","CB","HB"},
    {"ILE","CB","CG2"},
    {"ILE","CB","CG1"},
    {"ILE","CG2","HG21"},
    {"ILE","CG2","HG22"},
    {"ILE","CG2","HG23"},
    {"ILE","CG1","HG11"},
    {"ILE","CG1","HG12"},
    {"ILE","CG1","HG13"}, //added for 3msw
    {"ILE","CG1","CD1"},
    {"ILE","CD1","HD11"},
    {"ILE","CD1","HD12"},
    {"ILE","CD1","HD13"},
    {"ILE","CG1","CD"},
    {"ILE","CD","HD1"},
    {"ILE","CD","HD2"},
    {"ILE","CD","HD3"},
    {"ILE","C","O"},
    {"ILE","C","OT"},
    {"ILE","C","OT1"},
    {"ILE","C","OT2"},
    {"ILE","C","OXT"},
    {"ILE","C","O1"},
    {"ILE","C","O2"},
    {"ILE","C","HXT"},
    {"ILE","-C","N"},
    {"VAL","N","H"},
    {"VAL","N","HN"},
    {"VAL","N","HT1"},
    {"VAL","N","HT2"},
    {"VAL","N","HT3"},
    {"VAL","N","H1"},
    {"VAL","N","H2"},
    {"VAL","N","H3"},
    {"VAL","N","CA"},
    {"VAL","CA","HA"},
    {"VAL","CA","CB"},
    {"VAL","CA","C"},
    {"VAL","CB","HB"},
    {"VAL","CB","CG1"},
    {"VAL","CB","CG2"},
    {"VAL","CG1","HG11"},
    {"VAL","CG1","HG12"},
    {"VAL","CG1","HG13"},
    {"VAL","CG2","HG21"},
    {"VAL","CG2","HG22"},
    {"VAL","CG2","HG23"},
    {"VAL","C","O"},
    {"VAL","C","OT"},
    {"VAL","C","OT1"},
    {"VAL","C","OT2"},
    {"VAL","C","OXT"},
    {"VAL","C","O1"},
    {"VAL","C","O2"},
    {"VAL","C","HXT"},
    {"VAL","-C","N"},
    {"ASN","N","H"},
    {"ASN","N","HN"},
    {"ASN","N","HT1"},
    {"ASN","N","HT2"},
    {"ASN","N","HT3"},
    {"ASN","N","H1"},
    {"ASN","N","H2"},
    {"ASN","N","H3"},
    {"ASN","N","CA"},
    {"ASN","CA","HA"},
    {"ASN","CA","CB"},
    {"ASN","CA","C"},
    {"ASN","CB","HB1"},
    {"ASN","CB","HB2"},
    {"ASN","CB","HB3"}, //added for 3msw
    {"ASN","CB","CG"},
    {"ASN","CG","OD1"},
    {"ASN","CG","ND2"},
    {"ASN","ND2","HD21"},
    {"ASN","ND2","HD22"},
    {"ASN","ND2","HD2"},
    {"ASN","C","O"},
    {"ASN","C","OT"},
    {"ASN","C","OT1"},
    {"ASN","C","OT2"},
    {"ASN","C","OXT"}, //added for 8atc
    {"ASN","C","O1"},
    {"ASN","C","O2"},
    {"ASN","C","HXT"},
    {"ASN","-C","N"},
    {"GLN","N","H"},
    {"GLN","N","HN"},
    {"GLN","N","HT1"},
    {"GLN","N","HT2"},
    {"GLN","N","HT3"},
    {"GLN","N","H1"},
    {"GLN","N","H2"},
    {"GLN","N","H3"},
    {"GLN","N","CA"},
    {"GLN","CA","HA"},
    {"GLN","CA","CB"},
    {"GLN","CA","C"},
    {"GLN","CB","HB1"},
    {"GLN","CB","HB2"},
    {"GLN","CB","HB3"}, //added for 3msw
    {"GLN","CB","CG"},
    {"GLN","CG","HG1"},
    {"GLN","CG","HG2"},
    {"GLN","CG","HG3"}, //added for 3msw
    {"GLN","CG","CD"},
    {"GLN","CD","OE1"},
    {"GLN","CD","NE2"},
    {"GLN","NE2","HE21"},
    {"GLN","NE2","HE22"},
    {"GLN","C","O"},
    {"GLN","C","OT"},
    {"GLN","C","OT1"},
    {"GLN","C","OT2"},
    {"GLN","C","OXT"},
    {"GLN","C","O1"},
    {"GLN","C","O2"},
    {"GLN","C","HXT"},
    {"GLN","-C","N"},
    {"ARG","N","H"},
    {"ARG","N","HN"},
    {"ARG","N","HT1"},
    {"ARG","N","HT2"},
    {"ARG","N","HT3"},
    {"ARG","N","H1"},
    {"ARG","N","H2"},
    {"ARG","N","H3"},
    {"ARG","N","CA"},
    {"ARG","CA","HA"},
    {"ARG","CA","CB"},
    {"ARG","CA","C"},
    {"ARG","CB","HB1"},
    {"ARG","CB","HB2"},
    {"ARG","CB","HB3"}, //added for 3msw
    {"ARG","CB","CG"},
    {"ARG","CG","HG1"},
    {"ARG","CG","HG2"},
    {"ARG","CG","HG3"}, //added for 3msw
    {"ARG","CG","CD"},
    {"ARG","CD","HD1"},
    {"ARG","CD","HD2"},
    {"ARG","CD","HD3"}, //added for 3msw
    {"ARG","CD","NE"},
    {"ARG","NE","HE"},
    {"ARG","NE","CZ"},
    {"ARG","CZ","NH1"},
    {"ARG","CZ","NH2"},
    {"ARG","NH1","HH11"},
    {"ARG","NH1","HH12"},
    {"ARG","NH2","HH21"},
    {"ARG","NH2","HH22"},
    {"ARG","C","O"},
    {"ARG","C","OT"},
    {"ARG","C","OT1"},
    {"ARG","C","OT2"},
    {"ARG","C","OXT"}, //added for 2hhb
    {"ARG","C","O1"},
    {"ARG","C","O2"},
    {"ARG","C","HXT"},
    {"ARG","-C","N"},
    {"HIS","N","H"},
    {"HIS","N","HN"},
    {"HIS","N","HT1"},
    {"HIS","N","HT2"},
    {"HIS","N","HT3"},
    {"HIS","N","H1"},
    {"HIS","N","H2"},
    {"HIS","N","H3"},
    {"HIS","N","CA"},
    {"HIS","CA","HA"},
    {"HIS","CA","CB"},
    {"HIS","CA","C"},
    {"HIS","CB","HB1"},
    {"HIS","CB","HB2"},
    {"HIS","CB","HB3"}, //added for 3msw
    {"HIS","CB","CG"},
    {"HIS","CG","ND1"},
    {"HIS","CG","CD2"},
    {"HIS","ND1","HD1"},
    {"HIS","ND1","CE1"},
    {"HIS","CE1","HE1"},
    {"HIS","CE1","NE2"},
    {"HIS","NE2","CD2"},
    {"HIS","NE2","HE2"},
    {"HIS","CD2","HD2"},
    {"HIS","C","O"},
    {"HIS","C","OT"},
    {"HIS","C","OT1"},
    {"HIS","C","OT2"},
    {"HIS","C","OXT"},
    {"HIS","C","O1"},
    {"HIS","C","O2"},
    {"HIS","C","HXT"},
    {"HIS","-C","N"},
    {"HIE","N","H"},
    {"HIE","N","HN"},
    {"HIE","N","HT1"},
    {"HIE","N","HT2"},
    {"HIE","N","HT3"},
    {"HIE","N","H1"},
    {"HIE","N","H2"},
    {"HIE","N","H3"},
    {"HIE","N","CA"},
    {"HIE","CA","HA"},
    {"HIE","CA","CB"},
    {"HIE","CA","C"},
    {"HIE","CB","HB2"},
    {"HIE","CB","HB1"},
    {"HIE","CB","CG"},
    {"HIE","CG","ND1"},
    {"HIE","CG","CD2"},
    {"HIE","ND1","CE1"},
    {"HIE","CE1","HE1"},
    {"HIE","CE1","NE2"},
    {"HIE","NE2","HE2"},
    {"HIE","NE2","CD2"},
    {"HIE","CD2","HD2"},
    {"HIE","C","O"},
    {"HIE","C","OT"},
    {"HIE","C","OT1"},
    {"HIE","C","OT2"},
    {"HIE","C","OXT"},
    {"HIE","C","O1"},
    {"HIE","C","O2"},
    {"HIE","C","HXT"},
    {"HIE","-C","N"},
    {"HIP","N","H"},
    {"HIP","N","HN"},
    {"HIP","N","HT1"},
    {"HIP","N","HT2"},
    {"HIP","N","HT3"},
    {"HIP","N","H1"},
    {"HIP","N","H2"},
    {"HIP","N","H3"},
    {"HIP","N","CA"},
    {"HIP","CA","HA"},
    {"HIP","CA","CB"},
    {"HIP","CA","C"},
    {"HIP","CB","HB1"},
    {"HIP","CB","HB2"},
    {"HIP","CB","CG"},
    {"HIP","CG","ND1"},
    {"HIP","CG","CD2"},
    {"HIP","ND1","HD1"},
    {"HIP","ND1","CE1"},
    {"HIP","CE1","HE1"},
    {"HIP","CE1","NE2"},
    {"HIP","NE2","HE2"},
    {"HIP","NE2","CD2"},
    {"HIP","CD2","HD2"},
    {"HIP","C","O"},
    {"HIP","C","OT"},
    {"HIP","C","OT1"},
    {"HIP","C","OT2"},
    {"HIP","C","OXT"},
    {"HIP","C","O1"},
    {"HIP","C","O2"},
    {"HIP","C","HXT"},
    {"HIP","-C","N"},
    {"HSD","N","H"},
    {"HSD","N","HN"},
    {"HSD","N","HT1"},
    {"HSD","N","HT2"},
    {"HSD","N","HT3"},
    {"HSD","N","H1"},
    {"HSD","N","H2"},
    {"HSD","N","H3"},
    {"HSD","N","CA"},
    {"HSD","CA","HA"},
    {"HSD","CA","CB"},
    {"HSD","CA","C"},
    {"HSD","CB","HB1"},
    {"HSD","CB","HB2"},
    {"HSD","CB","CG"},
    {"HSD","CG","ND1"},
    {"HSD","CG","CD2"},
    {"HSD","ND1","HD1"},
    {"HSD","ND1","CE1"},
    {"HSD","CE1","HE1"},
    {"HSD","CE1","NE2"},
    {"HSD","NE2","CD2"},
    {"HSD","CD2","HD2"},
    {"HSD","C","O"},
    {"HSD","C","OT"},
    {"HSD","C","OT1"},
    {"HSD","C","OT2"},
    {"HSD","C","OXT"},
    {"HSD","C","O1"},
    {"HSD","C","O2"},
    {"HSD","C","HXT"},
    {"HSD","-C","N"},
    {"HSE","N","H"},
    {"HSE","N","HN"},
    {"HSE","N","HT1"},
    {"HSE","N","HT2"},
    {"HSE","N","HT3"},
    {"HSE","N","H1"},
    {"HSE","N","H2"},
    {"HSE","N","H3"},
    {"HSE","N","CA"},
    {"HSE","CA","HA"},
    {"HSE","CA","CB"},
    {"HSE","CA","C"},
    {"HSE","CB","HB1"},
    {"HSE","CB","HB2"},
    {"HSE","CB","CG"},
    {"HSE","CG","ND1"},
    {"HSE","CG","CD2"},
    {"HSE","ND1","HD1"},
    {"HSE","ND1","CE1"},
    {"HSE","CE1","HE1"},
    {"HSE","CE1","NE2"},
    {"HSE","NE2","CD2"},
    {"HSE","NE2","HE2"},
    {"HSE","CD2","HD2"},
    {"HSE","C","O"},
    {"HSE","C","OT"},
    {"HSE","C","OT1"},
    {"HSE","C","OT2"},
    {"HSE","C","OXT"},
    {"HSE","C","O1"},
    {"HSE","C","O2"},
    {"HSE","C","HXT"},
    {"HSE","-C","N"},
    {"HSP","N","H"},
    {"HSP","N","HN"},
    {"HSP","N","HT1"},
    {"HSP","N","HT2"},
    {"HSP","N","HT3"},
    {"HSP","N","H1"},
    {"HSP","N","H2"},
    {"HSP","N","H3"},
    {"HSP","N","CA"},
    {"HSP","CA","HA"},
    {"HSP","CA","CB"},
    {"HSP","CA","C"},
    {"HSP","CB","HB1"},
    {"HSP","CB","HB2"},
    {"HSP","CB","CG"},
    {"HSP","CG","ND1"},
    {"HSP","CG","CD2"},
    {"HSP","ND1","HD1"},
    {"HSP","ND1","CE1"},
    {"HSP","CE1","HE1"},
    {"HSP","CE1","NE2"},
    {"HSP","NE2","CD2"},
    {"HSP","CD2","HD2"},
    {"HSP","C","O"},
    {"HSP","C","OT"},
    {"HSP","C","OT1"},
    {"HSP","C","OT2"},
    {"HSP","C","OXT"},
    {"HSP","C","O1"},
    {"HSP","C","O2"},
    {"HSP","C","HXT"},
    {"HSP","-C","N"},
    {"TRP","N","H"},
    {"TRP","N","HN"},
    {"TRP","N","HT1"},
    {"TRP","N","HT2"},
    {"TRP","N","HT3"},
    {"TRP","N","H1"},
    {"TRP","N","H2"},
    {"TRP","N","H3"},
    {"TRP","N","CA"},
    {"TRP","CA","HA"},
    {"TRP","CA","CB"},
    {"TRP","CA","C"},
    {"TRP","CB","HB1"},
    {"TRP","CB","HB2"},
    {"TRP","CB","HB3"}, //added for 3msw
    {"TRP","CB","CG"},
    {"TRP","CG","CD1"},
    {"TRP","CG","CD2"},
    {"TRP","CD1","HD1"},
    {"TRP","CD1","NE1"},
    {"TRP","NE1","HE1"},
    {"TRP","NE1","CE2"},
    {"TRP","CE2","CZ2"},
    {"TRP","CE2","CD2"},
    {"TRP","CZ2","HZ2"},
    {"TRP","CZ2","CH2"},
    {"TRP","CH2","HH2"},
    {"TRP","CH2","CZ3"},
    {"TRP","CZ3","HZ3"},
    {"TRP","CZ3","CE3"},
    {"TRP","CE3","HE3"},
    {"TRP","CE3","CD2"},
    {"TRP","C","O"},
    {"TRP","C","OT"},
    {"TRP","C","OT1"},
    {"TRP","C","OT2"},
    {"TRP","C","OXT"},
    {"TRP","C","O1"},
    {"TRP","C","O2"},
    {"TRP","C","HXT"},
    {"TRP","-C","N"},
    {"PHE","N","H"},
    {"PHE","N","HN"},
    {"PHE","N","HT1"},
    {"PHE","N","HT2"},
    {"PHE","N","HT3"},
    {"PHE","N","H1"},
    {"PHE","N","H2"},
    {"PHE","N","H3"},
    {"PHE","N","CA"},
    {"PHE","CA","HA"},
    {"PHE","CA","CB"},
    {"PHE","CA","C"},
    {"PHE","CB","HB1"},
    {"PHE","CB","HB2"},
    {"PHE","CB","HB3"},
    {"PHE","CB","CG"},
    {"PHE","CG","CD1"},
    {"PHE","CG","CD2"},
    {"PHE","CD1","HD1"},
    {"PHE","CD1","CE1"},
    {"PHE","CE1","HE1"},
    {"PHE","CE1","CZ"},
    {"PHE","CZ","HZ"},
    {"PHE","CZ","CE2"},
    {"PHE","CE2","HE2"},
    {"PHE","CE2","CD2"},
    {"PHE","CD2","HD2"},
    {"PHE","C","O"},
    {"PHE","C","OT"},
    {"PHE","C","OT1"},
    {"PHE","C","OT2"},
    {"PHE","C","OXT"},
    {"PHE","C","O1"},
    {"PHE","C","O2"},
    {"PHE","C","HXT"},
    {"PHE","-C","N"},
    {"TYR","N","H"},
    {"TYR","N","HN"},
    {"TYR","N","HT1"},
    {"TYR","N","HT2"},
    {"TYR","N","HT3"},
    {"TYR","N","H1"},
    {"TYR","N","H2"},
    {"TYR","N","H3"},
    {"TYR","N","CA"},
    {"TYR","CA","HA"},
    {"TYR","CA","CB"},
    {"TYR","CA","C"},
    {"TYR","CB","HB1"},
    {"TYR","CB","HB2"},
    {"TYR","CB","HB3"}, //added for 3msw
    {"TYR","CB","CG"},
    {"TYR","CG","CD1"},
    {"TYR","CG","CD2"},
    {"TYR","CD1","HD1"},
    {"TYR","CD1","CE1"},
    {"TYR","CE1","HE1"},
    {"TYR","CE1","CZ"},
    {"TYR","CZ","OH"},
    {"TYR","CZ","CE2"},
    {"TYR","OH","HH"},
    {"TYR","CE2","HE2"},
    {"TYR","CE2","CD2"},
    {"TYR","CD2","HD2"},
    {"TYR","C","O"},
    {"TYR","C","OT"},
    {"TYR","C","OT1"},
    {"TYR","C","OT2"},
    {"TYR","C","OXT"},
    {"TYR","C","O1"},
    {"TYR","C","O2"},
    {"TYR","C","HXT"},
    {"TYR","-C","N"},
    {"GLU","N","H"},
    {"GLU","N","HN"},
    {"GLU","N","HT1"},
    {"GLU","N","HT2"},
    {"GLU","N","HT3"},
    {"GLU","N","H1"},
    {"GLU","N","H2"},
    {"GLU","N","H3"},
    {"GLU","N","CA"},
    {"GLU","CA","HA"},
    {"GLU","CA","CB"},
    {"GLU","CA","C"},
    {"GLU","CB","HB1"},
    {"GLU","CB","HB2"},
    {"GLU","CB","HB3"},
    {"GLU","CB","CG"},
    {"GLU","CG","HG1"},
    {"GLU","CG","HG2"},
    {"GLU","CG","HG3"},
    {"GLU","CG","CD"},
    {"GLU","CD","OE1"},
    {"GLU","CD","OE2"},
    {"GLU","OE1","HE1"},
    {"GLU","OE2","HE2"},
    {"GLU","C","O"},
    {"GLU","C","OT"},
    {"GLU","C","OT1"},
    {"GLU","C","OT2"},
    {"GLU","C","OXT"},
    {"GLU","C","O1"},
    {"GLU","C","O2"},
    {"GLU","C","HXT"},
    {"GLU","-C","N"},
    {"ASP","N","H"},
    {"ASP","N","HN"},
    {"ASP","N","HT1"},
    {"ASP","N","HT2"},
    {"ASP","N","HT3"},
    {"ASP","N","H1"},
    {"ASP","N","H2"},
    {"ASP","N","H3"},
    {"ASP","N","CA"},
    {"ASP","CA","HA"},
    {"ASP","CA","CB"},
    {"ASP","CA","C"},
    {"ASP","CB","HB1"},
    {"ASP","CB","HB2"},
    {"ASP","CB","HB3"},
    {"ASP","CB","CG"},
    {"ASP","CG","OD1"},
    {"ASP","CG","OD2"},
    {"ASP","OD1","HD1"},//added for ASP protonated state
    {"ASP","OD2","HD2"},//added for ASP protonated state
    {"ASP","C","O"},
    {"ASP","C","OT"},
    {"ASP","C","OT1"},
    {"ASP","C","OT2"},
    {"ASP","C","OXT"},
    {"ASP","C","O1"},
    {"ASP","C","O2"},
    {"ASP","C","HXT"},
    {"ASP","-C","N"},
    {"LYS","N","H"},
    {"LYS","N","HN"},
    {"LYS","N","HT1"},
    {"LYS","N","HT2"},
    {"LYS","N","HT3"},
    {"LYS","N","H1"},
    {"LYS","N","H2"},
    {"LYS","N","H3"},
    {"LYS","N","CA"},
    {"LYS","CA","HA"},
    {"LYS","CA","CB"},
    {"LYS","CA","C"},
    {"LYS","CB","HB1"},
    {"LYS","CB","HB2"},
    {"LYS","CB","HB3"},
    {"LYS","CB","CG"},
    {"LYS","CG","HG1"},
    {"LYS","CG","HG2"},
    {"LYS","CG","HG3"},
    {"LYS","CG","CD"},
    {"LYS","CD","HD1"},
    {"LYS","CD","HD2"},
    {"LYS","CD","HD3"},
    {"LYS","CD","CE"},
    {"LYS","CE","HE1"},
    {"LYS","CE","HE2"},
    {"LYS","CE","HE3"},
    {"LYS","CE","NZ"},
    {"LYS","NZ","HZ1"},
    {"LYS","NZ","HZ2"},
    {"LYS","NZ","HZ3"},
    {"LYS","C","O"},
    {"LYS","C","OT"},
    {"LYS","C","OT1"},
    {"LYS","C","OT2"},
    {"LYS","C","OXT"},
    {"LYS","C","O1"},
    {"LYS","C","O2"},
    {"LYS","C","HXT"},
    {"LYS","-C","N"},
    {"ORN","N","H"},
    {"ORN","N","HN"},
    {"ORN","N","HT1"},
    {"ORN","N","HT2"},
    {"ORN","N","HT3"},
    {"ORN","N","H1"},
    {"ORN","N","H2"},
    {"ORN","N","H3"},
    {"ORN","N","CA"},
    {"ORN","CA","HA"},
    {"ORN","CA","CB"},
    {"ORN","CA","C"},
    {"ORN","CB","HB1"},
    {"ORN","CB","HB2"},
    {"ORN","CB","CG"},
    {"ORN","CG","HG1"},
    {"ORN","CG","HG2"},
    {"ORN","CG","CD"},
    {"ORN","CD","HD1"},
    {"ORN","CD","HD2"},
    {"ORN","CD","NE"},
    {"ORN","NE","HE1"},
    {"ORN","NE","HE2"},
    {"ORN","NE","HE3"},
    {"ORN","C","O"},
    {"ORN","C","OT"},
    {"ORN","C","OT1"},
    {"ORN","C","OT2"},
    {"ORN","C","OXT"},
    {"ORN","C","O1"},
    {"ORN","C","O2"},
    {"ORN","C","HXT"},
    {"ORN","-C","N"},
    {"DAB","N","H"},
    {"DAB","N","HN"},
    {"DAB","N","HT1"},
    {"DAB","N","HT2"},
    {"DAB","N","HT3"},
    {"DAB","N","H1"},
    {"DAB","N","H2"},
    {"DAB","N","H3"},
    {"DAB","N","CA"},
    {"DAB","CA","HA"},
    {"DAB","CA","CB"},
    {"DAB","CA","C"},
    {"DAB","CB","HB1"},
    {"DAB","CB","HB2"},
    {"DAB","CB","CG"},
    {"DAB","CG","HG1"},
    {"DAB","CG","HG2"},
    {"DAB","CG","ND"},
    {"DAB","ND","HD1"},
    {"DAB","ND","HD2"},
    {"DAB","ND","HD3"},
    {"DAB","C","O"},
    {"DAB","C","OT"},
    {"DAB","C","OT1"},
    {"DAB","C","OT2"},
    {"DAB","C","OXT"},
    {"DAB","C","O1"},
    {"DAB","C","O2"},
    {"DAB","C","HXT"},
    {"DAB","-C","N"},
    {"LYN","N","H"},
    {"LYN","N","HN"},
    {"LYN","N","HT1"},
    {"LYN","N","HT2"},
    {"LYN","N","HT3"},
    {"LYN","N","H1"},
    {"LYN","N","H2"},
    {"LYN","N","H3"},
    {"LYN","N","CA"},
    {"LYN","CA","HA"},
    {"LYN","CA","CB"},
    {"LYN","CA","C"},
    {"LYN","CB","HB1"},
    {"LYN","CB","HB2"},
    {"LYN","CB","CG"},
    {"LYN","CG","HG1"},
    {"LYN","CG","HG2"},
    {"LYN","CG","CD"},
    {"LYN","CD","HD1"},
    {"LYN","CD","HD2"},
    {"LYN","CD","CE"},
    {"LYN","CE","HE1"},
    {"LYN","CE","HE2"},
    {"LYN","CE","NZ"},
    {"LYN","NZ","HZ1"},
    {"LYN","NZ","HZ2"},
    {"LYN","C","O"},
    {"LYN","C","OT"},
    {"LYN","C","OT1"},
    {"LYN","C","OT2"},
    {"LYN","C","OXT"},
    {"LYN","C","O1"},
    {"LYN","C","O2"},
    {"LYN","C","HXT"},
    {"LYN","-C","N"},
    {"PRO","N","CD"},
    {"PRO","N","CA"},
    {"PRO","N","H2"},
    {"PRO","N","H3"},
    {"PRO","CD","HD1"},
    {"PRO","CD","HD2"},
    {"PRO","CD","HD3"},
    {"PRO","CD","CG"},
    {"PRO","CG","HG1"},
    {"PRO","CG","HG2"},
    {"PRO","CG","HG3"},
    {"PRO","CG","CB"},
    {"PRO","CB","HB1"},
    {"PRO","CB","HB2"},
    {"PRO","CB","HB3"},
    {"PRO","CB","CA"},
    {"PRO","CA","HA1"},
    {"PRO","CA","HA"},
    {"PRO","CA","C"},
    {"PRO","C","O"},
    {"PRO","C","OT"},
    {"PRO","C","OT1"},
    {"PRO","C","OT2"},
    {"PRO","C","OXT"},
    {"PRO","C","O1"},
    {"PRO","C","O2"},
    {"PRO","C","HXT"},
    {"PRO","-C","N"},
    {"CYS","N","H"},
    {"CYS","N","HN"},
    {"CYS","N","HT1"},
    {"CYS","N","HT2"},
    {"CYS","N","HT3"},
    {"CYS","N","H1"},
    {"CYS","N","H2"},
    {"CYS","N","H3"},
    {"CYS","N","CA"},
    {"CYS","CA","HA"},
    {"CYS","CA","CB"},
    {"CYS","CA","C"},
    {"CYS","CB","HB1"},
    {"CYS","CB","HB2"},
    {"CYS","CB","HB3"},
    {"CYS","CB","SG"},
    {"CYS","SG","HG"},
    {"CYS","SG","HG1"},
    {"CYS","C","O"},
    {"CYS","C","OT"},
    {"CYS","C","OT1"},
    {"CYS","C","OT2"},
    {"CYS","C","OXT"},
    {"CYS","C","O1"},
    {"CYS","C","O2"},
    {"CYS","C","HXT"},
    {"CYS","-C","N"},
    {"CYX","N","H"},
    {"CYX","N","HN"},
    {"CYX","N","HT1"},
    {"CYX","N","HT2"},
    {"CYX","N","HT3"},
    {"CYX","N","H1"},
    {"CYX","N","H2"},
    {"CYX","N","H3"},
    {"CYX","N","CA"},
    {"CYX","CA","HA"},
    {"CYX","CA","CB"},
    {"CYX","CA","C"},
    {"CYX","CB","HB1"},
    {"CYX","CB","HB2"},
    {"CYX","CB","SG"},
    {"CYX","C","O"},
    {"CYX","C","OT"},
    {"CYX","C","OT1"},
    {"CYX","C","OT2"},
    {"CYX","C","OXT"},
    {"CYX","C","O1"},
    {"CYX","C","O2"},
    {"CYX","C","HXT"},
    {"CYX","-C","N"},
    {"CYS2","N","H"},
    {"CYS2","N","NH"},
    {"CYS2","N","HT1"},
    {"CYS2","N","HT2"},
    {"CYS2","N","HT3"},
    {"CYS2","N","H1"},
    {"CYS2","N","H2"},
    {"CYS2","N","H3"},
    {"CYS2","N","CA"},
    {"CYS2","CA","HA"},
    {"CYS2","CA","CB"},
    {"CYS2","CA","C"},
    {"CYS2","CB","HB1"},
    {"CYS2","CB","HB2"},
    {"CYS2","CB","SG"},
    {"CYS2","C","O"},
    {"CYS2","C","TO"},
    {"CYS2","C","T1O"},
    {"CYS2","C","T2O"},
    {"CYS2","C","OXT"},
    {"CYS2","C","1O"},
    {"CYS2","C","2O"},
    {"CYS2","C","HXT"},
    {"CYS2","-C","N"},
    {"MET","N","H"},
    {"MET","N","HN"},
    {"MET","N","HT1"},
    {"MET","N","HT2"},
    {"MET","N","HT3"},
    {"MET","N","H1"},
    {"MET","N","H2"},
    {"MET","N","H3"},
    {"MET","N","CA"},
    {"MET","CA","HA"},
    {"MET","CA","CB"},
    {"MET","CA","C"},
    {"MET","CB","HB1"},
    {"MET","CB","HB2"},
    {"MET","CB","HB3"}, //added for 2lao
    {"MET","CB","CG"},
    {"MET","CG","HG1"},
    {"MET","CG","HG2"},
    {"MET","CG","HG3"}, //added for 2lao
    {"MET","CG","SD"},
    {"MET","CG","S"}, //added for 3msw
    {"MET","SD","CE"},
    {"MET","S","CE"}, //added for 3msw
    {"MET","CE","HE1"},
    {"MET","CE","HE2"},
    {"MET","CE","HE3"},
    {"MET","C","O"},
    {"MET","C","OT"},
    {"MET","C","OT1"},
    {"MET","C","OT2"},
    {"MET","C","OXT"},
    {"MET","C","O1"},
    {"MET","C","O2"},
    {"MET","C","HXT"},
    {"MET","-C","N"},
    {"ASH","N","H"},
    {"ASH","N","HN"},
    {"ASH","N","HT1"},
    {"ASH","N","HT2"},
    {"ASH","N","HT3"},
    {"ASH","N","H1"},
    {"ASH","N","H2"},
    {"ASH","N","H3"},
    {"ASH","N","CA"},
    {"ASH","CA","HA"},
    {"ASH","CA","CB"},
    {"ASH","CA","C"},
    {"ASH","CB","HB1"},
    {"ASH","CB","HB2"},
    {"ASH","CB","CG"},
    {"ASH","CG","OD1"},
    {"ASH","CG","OD2"},
    {"ASH","OD2","HD2"},
    {"ASH","C","O"},
    {"ASH","C","OT"},
    {"ASH","C","OT1"},
    {"ASH","C","OT2"},
    {"ASH","C","OXT"},
    {"ASH","C","O1"},
    {"ASH","C","O2"},
    {"ASH","C","HXT"},
    {"ASH","-C","N"},
    {"GLH","N","H"},
    {"GLH","N","HN"},
    {"GLH","N","HT1"},
    {"GLH","N","HT2"},
    {"GLH","N","HT3"},
    {"GLH","N","H1"},
    {"GLH","N","H2"},
    {"GLH","N","H3"},
    {"GLH","N","CA"},
    {"GLH","CA","HA"},
    {"GLH","CA","CB"},
    {"GLH","CA","C"},
    {"GLH","CB","HB1"},
    {"GLH","CB","HB2"},
    {"GLH","CB","CG"},
    {"GLH","CG","HG1"},
    {"GLH","CG","HG2"},
    {"GLH","CG","CD"},
    {"GLH","CD","OE1"},
    {"GLH","CD","OE2"},
    {"GLH","OE2","HE2"},
    {"GLH","C","O"},
    {"GLH","C","OT"},
    {"GLH","C","OT1"},
    {"GLH","C","OT2"},
    {"GLH","C","OXT"},
    {"GLH","C","O1"},
    {"GLH","C","O2"},
    {"GLH","C","HXT"},
    {"GLH","-C","N"},
    {"G","P","-O3'"},
    {"G","P","OP1"},
    {"G","P","OP2"},
    {"G","P","O5'"},
    {"G","O5'","C5'"},
    {"G","O5'","HO5'"},
    {"G","C5'","H5'"},
    {"G","C5'","H5''"},
    {"G","C5'","C4'"},
    {"G","C4'","H4'"},
    {"G","C4'","O4'"},
    {"G","C4'","C3'"},
    {"G","C3'","H3'"},
    {"G","C3'","O3'"},
    {"G","C3'","C2'"},
    {"G","C2'","H2'"},
    {"G","C2'","O2'"},
    {"G","C2'","C1'"},
    {"G","O2'","HO2'"},
    {"G","C1'","H1'"},
    {"G","C1'","O4'"},
    {"G","C1'","N9"},
    {"G","O3'","HO3'"},
    {"G","N9","C4"},
    {"G","N9","C8"},
    {"G","C4","C5"},
    {"G","C4","N3"},
    {"G","N3","C2"},
    {"G","C2","N1"},
    {"G","C2","N2"},
    {"G","N2","H21"},
    {"G","N2","H22"},
    {"G","N1","H1"},
    {"G","N1","C6"},
    {"G","C6","O6"},
    {"G","C6","C5"},
    {"G","C5","N7"},
    {"G","N7","C8"},
    {"G","C8","H8"},
    {"A","P","-O3'"},
    {"A","P","OP1"},
    {"A","P","OP2"},
    {"A","P","O5'"},
    {"A","O5'","C5'"},
    {"A","O5'","HO5'"},
    {"A","C5'","H5'"},
    {"A","C5'","H5''"},
    {"A","C5'","C4'"},
    {"A","C4'","H4'"},
    {"A","C4'","O4'"},
    {"A","C4'","C3'"},
    {"A","C3'","H3'"},
    {"A","C3'","O3'"},
    {"A","C3'","C2'"},
    {"A","C2'","H2'"},
    {"A","C2'","O2'"},
    {"A","C2'","C1'"},
    {"A","O2'","HO2'"},
    {"A","C1'","H1'"},
    {"A","C1'","O4'"},
    {"A","C1'","N9"},
    {"A","O3'","HO3'"},
    {"A","N9","C8"},
    {"A","N9","C4"},
    {"A","C4","C5"},
    {"A","C4","N3"},
    {"A","N3","C2"},
    {"A","C2","H2"},
    {"A","C2","N1"},
    {"A","N1","C6"},
    {"A","C6","N6"},
    {"A","C6","C5"},
    {"A","N6","H62"},
    {"A","N6","H61"},
    {"A","C5","N7"},
    {"A","N7","C8"},
    {"A","C8","H8"},
    {"C","P","-O3'"},
    {"C","P","OP1"},
    {"C","P","OP2"},
    {"C","P","O5'"},
    {"C","O5'","C5'"},
    {"C","O5'","HO5'"},
    {"C","C5'","H5'"},
    {"C","C5'","H5''"},
    {"C","C5'","C4'"},
    {"C","C4'","H4'"},
    {"C","C4'","O4'"},
    {"C","C4'","C3'"},
    {"C","C3'","H3'"},
    {"C","C3'","O3'"},
    {"C","C3'","C2'"},
    {"C","C2'","H2'"},
    {"C","C2'","O2'"},
    {"C","C2'","C1'"},
    {"C","O2'","HO2'"},
    {"C","C1'","H1'"},
    {"C","C1'","O4'"},
    {"C","C1'","N1"},
    {"C","O3'","HO3'"},
    {"C","N1","C6"},
    {"C","N1","C2"},
    {"C","C2","O2"},
    {"C","C2","N3"},
    {"C","N3","C4"},
    {"C","C4","N4"},
    {"C","N4","H41"},
    {"C","N4","H42"},
    {"C","C4","C5"},
    {"C","C5","H5"},
    {"C","C5","C6"},
    {"C","C6","H6"},
    {"U","P","-O3'"},
    {"U","P","OP1"},
    {"U","P","OP2"},
    {"U","P","O5'"},
    {"U","O5'","C5'"},
    {"U","O5'","HO5'"},
    {"U","C5'","H5'"},
    {"U","C5'","H5''"},
    {"U","C5'","C4'"},
    {"U","C4'","H4'"},
    {"U","C4'","O4'"},
    {"U","C4'","C3'"},
    {"U","C3'","H3'"},
    {"U","C3'","O3'"},
    {"U","C3'","C2'"},
    {"U","C2'","H2'"},
    {"U","C2'","O2'"},
    {"U","C2'","C1'"},
    {"U","O2'","HO2'"},
    {"U","C1'","H1'"},
    {"U","C1'","O4'"},
    {"U","C1'","N1"},
    {"U","O3'","HO3'"},
    {"U","N1","C6"},
    {"U","N1","C2"},
    {"U","C2","O2"},
    {"U","C2","N3"},
    {"U","N3","H3"},
    {"U","N3","C4"},
    {"U","C4","O4"},
    {"U","C4","C5"},
    {"U","C5","H5"},
    {"U","C5","C6"},
    {"U","C6","H6"},

    //DNA nucleotides
    {"DG","P","-O3'"},
    {"DG","P","OP1"},
    {"DG","P","OP2"},
    {"DG","P","O5'"},
    {"DG","O5'","C5'"},
    {"DG","O5'","HO5'"},
    {"DG","C5'","H5'"},
    {"DG","C5'","H5''"},
    {"DG","C5'","C4'"},
    {"DG","C4'","H4'"},
    {"DG","C4'","O4'"},
    {"DG","C4'","C3'"},
    {"DG","C3'","H3'"},
    {"DG","C3'","O3'"},
    {"DG","C3'","C2'"},
    {"DG","C2'","H2'"},
    {"DG","C2'","H2''"},
    {"DG","C2'","C1'"},
    {"DG","C1'","H1'"},
    {"DG","C1'","O4'"},
    {"DG","C1'","N9"},
    {"DG","O3'","HO3'"},
    {"DG","N9","C4"},
    {"DG","N9","C8"},
    {"DG","C4","C5"},
    {"DG","C4","N3"},
    {"DG","N3","C2"},
    {"DG","C2","N1"},
    {"DG","C2","N2"},
    {"DG","N2","H21"},
    {"DG","N2","H22"},
    {"DG","N1","H1"},
    {"DG","N1","C6"},
    {"DG","C6","O6"},
    {"DG","C6","C5"},
    {"DG","C5","N7"},
    {"DG","N7","C8"},
    {"DG","C8","H8"},
    {"DA","P","-O3'"},
    {"DA","P","OP1"},
    {"DA","P","OP2"},
    {"DA","P","O5'"},
    {"DA","O5'","C5'"},
    {"DA","O5'","HO5'"},
    {"DA","C5'","H5'"},
    {"DA","C5'","H5''"},
    {"DA","C5'","C4'"},
    {"DA","C4'","H4'"},
    {"DA","C4'","O4'"},
    {"DA","C4'","C3'"},
    {"DA","C3'","H3'"},
    {"DA","C3'","O3'"},
    {"DA","C3'","C2'"},
    {"DA","C2'","H2'"},
    {"DA","C2'","H2''"},
    {"DA","C2'","C1'"},
    {"DA","C1'","H1'"},
    {"DA","C1'","O4'"},
    {"DA","C1'","N9"},
    {"DA","O3'","HO3'"},
    {"DA","N9","C8"},
    {"DA","N9","C4"},
    {"DA","C4","C5"},
    {"DA","C4","N3"},
    {"DA","N3","C2"},
    {"DA","C2","H2"},
    {"DA","C2","N1"},
    {"DA","N1","C6"},
    {"DA","C6","N6"},
    {"DA","C6","C5"},
    {"DA","N6","H62"},
    {"DA","N6","H61"},
    {"DA","C5","N7"},
    {"DA","N7","C8"},
    {"DA","C8","H8"},
    {"DC","P","-O3'"},
    {"DC","P","OP1"},
    {"DC","P","OP2"},
    {"DC","P","O5'"},
    {"DC","O5'","C5'"},
    {"DC","O5'","HO5'"},
    {"DC","C5'","H5'"},
    {"DC","C5'","H5''"},
    {"DC","C5'","C4'"},
    {"DC","C4'","H4'"},
    {"DC","C4'","O4'"},
    {"DC","C4'","C3'"},
    {"DC","C3'","H3'"},
    {"DC","C3'","O3'"},
    {"DC","C3'","C2'"},
    {"DC","C2'","H2'"},
    {"DC","C2'","H2''"},
    {"DC","C2'","C1'"},
    {"DC","C1'","H1'"},
    {"DC","C1'","O4'"},
    {"DC","C1'","N1"},
    {"DC","O3'","HO3'"},
    {"DC","N1","C6"},
    {"DC","N1","C2"},
    {"DC","C2","O2"},
    {"DC","C2","N3"},
    {"DC","N3","C4"},
    {"DC","C4","N4"},
    {"DC","N4","H41"},
    {"DC","N4","H42"},
    {"DC","C4","C5"},
    {"DC","C5","H5"},
    {"DC","C5","C6"},
    {"DC","C6","H6"},
    {"DT","P","-O3'"},
    {"DT","P","OP1"},
    {"DT","P","OP2"},
    {"DT","P","O5'"},
    {"DT","O5'","C5'"},
    {"DT","O5'","HO5'"},
    {"DT","C5'","H5'"},
    {"DT","C5'","H5''"},
    {"DT","C5'","C4'"},
    {"DT","C4'","H4'"},
    {"DT","C4'","O4'"},
    {"DT","C4'","C3'"},
    {"DT","C3'","H3'"},
    {"DT","C3'","O3'"},
    {"DT","C3'","C2'"},
    {"DT","C2'","H2'"},
    {"DT","C2'","H2''"},
    {"DT","C2'","C1'"},
    {"DT","C1'","H1'"},
    {"DT","C1'","O4'"},
    {"DT","C1'","N1"},
    {"DT","O3'","HO3'"},
    {"DT","N1","C6"},
    {"DT","N1","C2"},
    {"DT","C2","O2"},
    {"DT","C2","N3"},
    {"DT","N3","H3"},
    {"DT","N3","C4"},
    {"DT","C4","O4"},
    {"DT","C4","C5"},
    {"DT","C5","H5"},
    {"DT","C5","C6"},
    {"DT","C6","H6"},
    {"DT","C5","C7"},
    {"DT","C7","H71"},
    {"DT","C7","H72"},
    {"DT","C7","H73"},
    {"MG","MG","MG"},

    //Non-standard residues and ligands
    {"CSO","N","CA"},
    {"CSO","CA","HA"},
    {"CSO","CA","CB"},
    {"CSO","CA","C"},
    {"CSO","CB","SG"},
    {"CSO","CB","HB"},
    {"CSO","CB","HB1"},
    {"CSO","CB","HB2"},
    {"CSO","CB","HB3"},
    {"CSO","SG","OD"},
    {"CSO","OD","HD"},
    {"CSO","C","O"},
    {"CSO","-C","N"},

    //Extension to waters
    {"HOH","O","H"},
    {"HOH","O","H1"},
    {"HOH","O","H2"},
    
    {"END", "",""}//Must always be LAST
};


const char* const FIXED_BOND_PROFILES [][3] = {
    {"ALA","-C","N"},
    {"ARG","-C","N"},
    //{"ARG","CD","NE"}, //in KINARI
    {"ARG","NE","CZ"}, //in ccp4, in KINARI, but 99% at 180 deg rotation
    {"ARG","CZ","NH1"}, //added
    {"ARG","CZ","NH2"}, //added
    {"ASN","-C","N"},
    {"ASN","CG","OD1"}, //added
    {"ASN","CG","ND2"}, //added
    {"ASP","-C","N"},
    {"ASP","CG","OD1"}, //added
    {"ASP","CG","OD2"}, //added
    {"CYS","-C","N"},
    {"CYX","-C","N"},
    {"CYS2","-C","N"},
    {"GLN","-C","N"},
    {"GLN","CD","OE1"}, //added
    {"GLN","CD","NE2"}, //added
    {"GLU","-C","N"},
    {"GLU","CD","OE1"}, //added
    {"GLU","CD","OE2"}, //added
    {"GLY","-C","N"},
    {"HIS","-C","N"},
    {"HIS","CG","CD2"},
    {"HIS","CG","ND1"},
    {"HIS","ND1","CE1"},
    {"HIS","CD2","NE2"},
    {"HIS","CE1","NE2"},
    //{"HIS","CB","CG"}, //in KINARI
    {"ILE","-C","N"},
    {"ILE","CB","CG2"},
    {"ILE","CG1","CD1"},
    {"LEU","-C","N"},
    {"LEU","CG","CD1"},
    {"LEU","CG","CD2"},
    {"LYS","-C","N"},
    {"MET","-C","N"},
    {"PHE","-C","N"},
    {"PHE","CG","CD1"},
    {"PHE","CG","CD2"},
    {"PHE","CD1","CE1"},
    {"PHE","CE1","CZ"},
    {"PHE","CZ","CE2"},
    {"PHE","CE2","CD2"},
    //Makes sure prolines are considered rigid bodies
    {"PRO","-C","N"},
    {"PRO","N","CD"}, //added
    {"PRO","CA","CB"},
    {"PRO","CB","CG"},
    {"PRO","CG","CD"},
    {"PRO","CD","N"},
    {"SER","-C","N"},
    {"THR","-C","N"},
    {"TRP","-C","N"},
    {"TRP","CG","CD1"},
    {"TRP","CG","CD2"},
    {"TRP","CD","OE1"},
    {"TRP","CD1","NE1"},
    {"TRP","NE1","CE2"},
    {"TRP","CE2","CZ2"},
    {"TRP","CE2","CD2"},
    {"TRP","CZ2","CH2"},
    {"TRP","CH2","CZ3"},
    {"TRP","CZ3","CE3"},
    {"TRP","CZ3","CH2"}, //added
    {"TRP","CE3","CD2"},
    {"TYR","-C","N"},
    {"TYR","CG","CD1"},
    {"TYR","CG","CD2"},
    {"TYR","CD1","CE1"},
    {"TYR","CE1","CZ"},
    {"TYR","CZ","CE2"},
    {"TYR","CE2","CD2"},
    {"VAL","-C","N"},

    //RNA and DNA
    {"G","N9","C4"},
    {"G","N9","C8"},
    {"G","C4","C5"},
    {"G","C4","N3"},
    {"G","N3","C2"},
    {"G","C2","N2"},
    {"G","N1","C6"},
    {"G","C6","C5"},
    {"G","C5","N7"},
    {"G","N7","C8"},

    {"A","N9","C8"},
    {"A","N9","C4"},
    {"A","C4","C5"},
    {"A","C4","N3"},
    {"A","N3","C2"},
    {"A","C2","N1"},
    {"A","N1","C6"},
    {"A","C6","C5"},
    {"A","C6","N6"},
    {"A","C5","N7"},
    {"A","N7","C8"},

    {"C","N1","C6"},
    {"C","N1","C2"},
    {"C","C2","N3"},
    {"C","N3","C4"},
    {"C","C4","C5"},
    {"C","C4","N4"},
    {"C","C5","C6"},

    {"U","N1","C6"},
    {"U","N1","C2"},
    {"U","C2","N3"},
    {"U","N3","C4"},
    {"U","C4","C5"},
    {"U","C5","C6"},

    {"DG","N9","C4"},
    {"DG","N9","C8"},
    {"DG","C4","C5"},
    {"DG","C4","N3"},
    {"DG","N3","C2"},
    {"DG","C2","N2"},
    {"DG","N1","C6"},
    {"DG","C6","C5"},
    {"DG","C5","N7"},
    {"DG","N7","C8"},

    {"DA","N9","C8"},
    {"DA","N9","C4"},
    {"DA","C4","C5"},
    {"DA","C4","N3"},
    {"DA","N3","C2"},
    {"DA","C2","N1"},
    {"DA","N1","C6"},
    {"DA","C6","C5"},
    {"DA","C6","N6"},
    {"DA","C5","N7"},
    {"DA","N7","C8"},

    {"DC","N1","C6"},
    {"DC","N1","C2"},
    {"DC","C2","N3"},
    {"DC","N3","C4"},
    {"DC","C4","C5"},
    {"DC","C4","N4"},
    {"DC","C5","C6"},

    {"DT","N1","C6"},
    {"DT","N1","C2"},
    {"DT","C2","N3"},
    {"DT","N3","C4"},
    {"DT","C4","C5"},
    {"DT","C5","C6"},

    //Makes sure riboses are considered rigid bodies
    {"G","C1'","C2'"},
    {"G","C2'","C3'"},
    {"G","C3'","C4'"},
    {"G","C4'","O4'"},
    {"G","O4'","C1'"},
    {"A","C1'","C2'"},
    {"A","C2'","C3'"},
    {"A","C3'","C4'"},
    {"A","C4'","O4'"},
    {"A","O4'","C1'"},
    {"C","C1'","C2'"},
    {"C","C2'","C3'"},
    {"C","C3'","C4'"},
    {"C","C4'","O4'"},
    {"C","O4'","C1'"},
    {"U","C1'","C2'"},
    {"U","C2'","C3'"},
    {"U","C3'","C4'"},
    {"U","C4'","O4'"},
    {"U","O4'","C1'"},
    {"DG","C1'","C2'"},
    {"DG","C2'","C3'"},
    {"DG","C3'","C4'"},
    {"DG","C4'","O4'"},
    {"DG","O4'","C1'"},
    {"DA","C1'","C2'"},
    {"DA","C2'","C3'"},
    {"DA","C3'","C4'"},
    {"DA","C4'","O4'"},
    {"DA","O4'","C1'"},
    {"DC","C1'","C2'"},
    {"DC","C2'","C3'"},
    {"DC","C3'","C4'"},
    {"DC","C4'","O4'"},
    {"DC","O4'","C1'"},
    {"DT","C1'","C2'"},
    {"DT","C2'","C3'"},
    {"DT","C3'","C4'"},
    {"DT","C4'","O4'"},
    {"DT","O4'","C1'"},
    
    //Makes sure that C-O bonds are rigid
    {"URE","C","O"},
    {"ACE","C","O"},
    {"ALA","C","O"},
    {"GLY","C","O"},
    {"SER","C","O"},
    {"THR","C","O"},
    {"LEU","C","O"},
    {"ILE","C","O"},
    {"VAL","C","O"},
    {"ASN","C","O"},
    {"GLN","C","O"},
    {"ARG","C","O"},
    {"HIS","C","O"},
    {"HIE","C","O"},
    {"HIP","C","O"},
    {"HSD","C","O"},
    {"HSE","C","O"},
    {"HSP","C","O"},
    {"TRP","C","O"},
    {"PHE","C","O"},
    {"TYR","C","O"},
    {"GLU","C","O"},
    {"ASP","C","O"},
    {"LYS","C","O"},
    {"ORN","C","O"},
    {"DAB","C","O"},
    {"LYN","C","O"},
    {"PRO","C","O"},
    {"CYS","C","O"},
    {"CYX","C","O"},
    {"CYS2","C","O"},
    {"MET","C","O"},
    {"ASH","C","O"},
    {"GLH","C","O"},
    {"ARN","C","O"},
    {"ASH","C","O"},
    {"CSO","C","O"},
    {"LYN","C","O"},
    {"ORN","C","O"},

    //Todo: Terminal bonds to OT, OXT etc.
    
    //Non-standard residues / protonation states / ligands
    {"ARN","-C","N"},
    {"ARN","CZ","NH1"}, //added
    {"ARN","CZ","NH2"}, //added
    {"ASH","-C","N"},
    {"ASH","-C","N"},
    {"ASH","CG","OD1"}, //added
    {"ASH","CG","OD2"}, //added
    {"CSO","-C","N"},
    {"DAB","-C","N"},
    {"GLH","-C","N"},
    {"GLH","CD","OE1"}, //added
    {"GLH","CD","OE2"}, //added
    {"HIE","-C","N"},
    {"HIE","CG","CD2"},
    {"HIE","CG","ND1"},
    {"HIE","ND1","CE1"},
    {"HIE","CD2","NE2"},
    {"HIE","CE1","NE2"},
    {"HIP","-C","N"},
    {"HIP","CG","CD2"},
    {"HIP","CG","ND1"},
    {"HIP","ND1","CE1"},
    {"HIP","CD2","NE2"},
    {"HIP","CE1","NE2"},
    {"HSD","-C","N"},
    {"HSD","CG","CD2"},
    {"HSD","CG","ND1"},
    {"HSD","ND1","CE1"},
    {"HSD","CD2","NE2"},
    {"HSD","CE1","NE2"},
    {"HSE","-C","N"},
    {"HSE","CG","CD2"},
    {"HSE","CG","ND1"},
    {"HSE","ND1","CE1"},
    {"HSE","CD2","NE2"},
    {"HSE","CE1","NE2"},
    {"HSP","-C","N"},
    {"HSP","CG","CD2"},
    {"HSP","CG","ND1"},
    {"HSP","ND1","CE1"},
    {"HSP","CD2","NE2"},
    {"HSP","CE1","NE2"},
    {"LYN","-C","N"},
    {"ORN","-C","N"},
    
    {"END","",""}//Must always be LAST
};



