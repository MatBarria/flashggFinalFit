# Python file to store systematics: for STXS analysis

# Comment out all nuisances that you do not want to include

# THEORY SYSTEMATICS:

# For type:constant
#  1) specify same value for all processes
#  2) define process map json in ./theory_uncertainties (add process names where necessary!)

# For type:factory
# Tier system: adds different uncertainties to dataframe
#   1) shape: absolute yield of process kept constant, shape effects i.e. calc migrations across cats
#   2) ishape: as (1) but absolute yield for proc x cat is allowed to vary
#   3) norm: absolute yield of production mode (s0) kept constant but migrations across sub-processes e.g. STXS bins.
#      Same value in each category.
#   4) inorm: as (3) but absolute yield for proc x cat is allowed to vary
#   5) inc: variations in production mode (s0), same value for each subprocess in each category
# Relations: shape = ishape/inorm
#            norm  = inorm/inc
# Specify as list in dict: e.g. 'tiers'=['inc','inorm','norm','ishape','shape']


theory_systematics = [
    {
        "name": "BR_hmm",
        "title": "BR_hmm",
        "type": "constant",
        "prior": "lnN",
        "correlateAcrossYears": 1,
        "value": "0.983/1.0167",
    },
    {
        "name": "QCDscale_ggH",
        "title": "thu_QCDscale_ggH",
        "type": "constant",
        "prior": "lnN",
        "correlateAcrossYears": 1,
        "value": "theory_uncertainties/thu_ggh.json",
    },
    {
        "name": "pdf_Higgs_ggH",
        "title": "thu_pdf_Higgs_ggH",
        "type": "constant",
        "prior": "lnN",
        "correlateAcrossYears": 1,
        "value": "theory_uncertainties/thu_ggh.json",
    },
    {
        "name": "alphaS_ggH",
        "title": "alphaS_ggH",
        "type": "constant",
        "prior": "lnN",
        "correlateAcrossYears": 1,
        "value": "theory_uncertainties/thu_ggh.json",
    },
    {
        "name": "QCDscale_qqH",
        "title": "thu_QCDscale_qqH",
        "type": "constant",
        "prior": "lnN",
        "correlateAcrossYears": 1,
        "value": "theory_uncertainties/thu_qqh.json",
    },
    {
        "name": "pdf_Higgs_qqH",
        "title": "thu_pdf_Higgs_qqH",
        "type": "constant",
        "prior": "lnN",
        "correlateAcrossYears": 1,
        "value": "theory_uncertainties/thu_qqh.json",
    },
    {
        "name": "alphaS_qqH",
        "title": "alphaS_qqH",
        "type": "constant",
        "prior": "lnN",
        "correlateAcrossYears": 1,
        "value": "theory_uncertainties/thu_qqh.json",
    },

    # Shape uncertainties: enter direct XS measurements
    # Scale weights grouping is defined in makeDatacard.py
    # For some reason, the name is saved only as `Scal`, not `Scale`.
    # Comment out the nominal weight here as it does not contain any `tiers`,
    # so it would fail in `makeDatacard.py`.
    # The scheme below is valid for v13; explicitly check the NanoAOD
    # documentation to validate your setup.

    # {'name':'weight_LHEScal_0','title':'CMS_hgg_scaleWeight_0','type':'factory','prior':'lnN','correlateAcrossYears':1,'tiers':['shape']},
    # {'name':'weight_LHEScal_1','title':'CMS_hgg_scaleWeight_1','type':'factory','prior':'lnN','correlateAcrossYears':1,'tiers':['shape']},
    # {'name':'weight_LHEScal_2','title':'CMS_hgg_scaleWeight_2','type':'factory','prior':'lnN','correlateAcrossYears':1,'tiers':['shape']}, # Unphysical
    # {'name':'weight_LHEScal_3','title':'CMS_hgg_scaleWeight_3','type':'factory','prior':'lnN','correlateAcrossYears':1,'tiers':['shape']},
    # {'name':'weight_LHEScal_4','title':'CMS_hgg_scaleWeight_4','type':'factory','prior':'lnN','correlateAcrossYears':1,'tiers':['shape']}, # nominal weight
    # {'name':'weight_LHEScal_5','title':'CMS_hgg_scaleWeight_5','type':'factory','prior':'lnN','correlateAcrossYears':1,'tiers':['shape']},
    # {'name':'weight_LHEScal_6','title':'CMS_hgg_scaleWeight_6','type':'factory','prior':'lnN','correlateAcrossYears':1,'tiers':['shape']}, # Unphysical
    # {'name':'weight_LHEScal_7','title':'CMS_hgg_scaleWeight_7','type':'factory','prior':'lnN','correlateAcrossYears':1,'tiers':['shape']},
    # {'name':'weight_LHEScal_8','title':'CMS_hgg_scaleWeight_8','type':'factory','prior':'lnN','correlateAcrossYears':1,'tiers':['shape']},
    # {'name':'weight_AlphaS','title':'CMS_hgg_AlphaS','type':'factory','prior':'lnN','correlateAcrossYears':1,'tiers':['shape']},
    # {'name':'weight_PS_ISR','title':'CMS_hgg_PS_ISR','type':'factory','prior':'lnN','correlateAcrossYears':1,'tiers':['shape']},
    # {'name':'weight_PS_FSR','title':'CMS_hgg_PS_FSR','type':'factory','prior':'lnN','correlateAcrossYears':1,'tiers':['shape']},
]

# PDF weight
# for i in range(1,101):
#     theory_systematics.append(
#         {
#             'name': 'weight_LHEPd_%g' % i,
#             'title': 'CMS_hgg_pdfWeight_%g' % i,
#             'type': 'factory',
#             'prior': 'lnN',
#             'correlateAcrossYears': 1,
#             'tiers': ['shape'],
#         }
#     )


# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# EXPERIMENTAL SYSTEMATICS
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#
# correlateAcrossYears = 0  : no correlation
# correlateAcrossYears = 1  : fully correlated
# correlateAcrossYears = -1 : partially correlated
#
# The category/process-dependent numbers are no longer stored directly in this
# Python file. They are read from the JSON files produced by the standalone
# systematic-calculation scripts.
#
# Expected JSON structure:
#
# {
#     "ggH": {
#         "categories": {
#             "ggHcat1": {
#                 "mu_trig_eff": [down, up],
#                 ...
#             }
#         }
#     },
#     "qqH": {
#         "categories": {
#             "ggHcat1": {
#                 "mu_trig_eff": [down, up],
#                 ...
#             }
#         }
#     }
# }
#
# IMPORTANT:
# The keys stored inside the JSON must match the systematic "name", not
# necessarily the datacard "title". For example:
#
#   name = "scale"       -> JSON key "scale"
#   title = "CMS_scale_m"
#
#   name = "smearing"    -> JSON key "smearing"
#   title = "CMS_res_m"
#


MUON_EFF_JSON = "experimental_systematics/muon_eff_systematics.json"
PT_JSON  = "experimental_systematics/pt_systematics.json"
PILEUP_JSON = "experimental_systematics/pileup_systematics.json"


def json_constant_systematic(name, title, json_file):
    """Build a fully-correlated constant lnN nuisance read from JSON."""
    return {
        "name": name,
        "title": title,
        "type": "constant",
        "prior": "lnN",
        "correlateAcrossYears": 1,
        "value": json_file,
    }



experimental_systematics = [
    # Integrated luminosity
    {
        "name": "lumi_combined",
        "title": "CMS_lumi",
        "type": "constant",
        "prior": "lnN",
        "correlateAcrossYears": 1,
        "value": "1.03",
    },

    # Muon efficiency scale factors
    json_constant_systematic(
        "mu_trig_eff",
        "CMS_mu_trig_eff",
        MUON_EFF_JSON,
    ),
    json_constant_systematic(
        "mu_ID_eff",
        "CMS_mu_ID_eff",
        MUON_EFF_JSON,
    ),
    json_constant_systematic(
        "mu_ISO_eff",
        "CMS_mu_ISO_eff",
        MUON_EFF_JSON,
    ),

    json_constant_systematic(
        "pileup",
        "CMS_pileup",
        PILEUP_JSON,
    ),
    # Muon pT scale / resolution
    json_constant_systematic(
        "scale",
        "CMS_scale_m",
        PT_JSON ,
    ),
    json_constant_systematic(
        "smearing",
        "CMS_res_m",
        PT_JSON ,
    ),
]


# JES nuisances correlated across years.
JES_CORRELATED = [
    "CMS_scale_j_FlavorQCD",
    "CMS_scale_j_RelativeBal",
    "CMS_scale_j_HF",
    "CMS_scale_j_BBEC1",
    "CMS_scale_j_EC2",
    "CMS_scale_j_Absolute",
]

for systematic_name in JES_CORRELATED:
    experimental_systematics.append(
        json_constant_systematic(
            systematic_name,
            systematic_name,
            PT_JSON ,
        )
    )


# JES nuisances specific to each Run-3 era.
JES_ERA_COMPONENTS = [
    "BBEC1",
    "RelativeSample",
    "EC2",
    "HF",
    "Absolute",
]

JES_ERAS = [
    "2022",
    "2022EE",
    "2023",
    "2023BPix",
    "2024",
    "2025",
]

for era in JES_ERAS:
    for component in JES_ERA_COMPONENTS:
        systematic_name = f"CMS_scale_j_{component}_{era}"
        experimental_systematics.append(
            json_constant_systematic(
                systematic_name,
                systematic_name,
                PT_JSON ,
            )
        )


# JER nuisances specific to each Run-3 era and eta/pT region.
JER_COMPONENTS = [
    "eta0to1p93_pt0toInf",
    "eta1p93to2p5_pt0toInf",
    "eta2p5to3p0_pt0to50",
    "eta2p5to3p0_pt50toInf",
    "eta3to5_pt0to50",
    "eta3to5_pt50toInf",
]

for era in JES_ERAS:
    for component in JER_COMPONENTS:
        systematic_name = f"CMS_res_j_{component}_{era}"
        experimental_systematics.append(
            json_constant_systematic(
                systematic_name,
                systematic_name,
                PT_JSON ,
            )
        )


# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# SIGNAL SHAPE SYSTEMATICS
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Shape nuisances: effect encoded in signal model
# mode = (other,scalesGlobal,scales,scalesCorr,smears):
# match the definition in the signal models

signal_shape_systematics = [
  #  {
  #      "name": "peak_ggH_ggHcat1",
  #      "title": "peak_ggH_ggHcat1",
  #      "type": "signal_shape",
  #      "mode": "other",
  #      "mean": "0.0",
  #      "sigma": "0.001",
  #      "cats": ["ggHcat1"],
  #  },
  #  {
  #      "name": "peak_ggH_ggHcat2",
  #      "title": "peak_ggH_ggHcat2",
  #      "type": "signal_shape",
  #      "mode": "other",
  #      "mean": "0.0",
  #      "sigma": "0.001",
  #      "cats": ["ggHcat2"],
  #  },
  #  {
  #      "name": "peak_ggH_ggHcat3",
  #      "title": "peak_ggH_ggHcat3",
  #      "type": "signal_shape",
  #      "mode": "other",
  #      "mean": "0.0",
  #      "sigma": "0.001",
  #      "cats": ["ggHcat3"],
  #  },
  #  {
  #      "name": "peak_ggH_ggHcat4",
  #      "title": "peak_ggH_ggHcat4",
  #      "type": "signal_shape",
  #      "mode": "other",
  #      "mean": "0.0",
  #      "sigma": "0.001",
  #      "cats": ["ggHcat4"],
  #  },
  #  {
  #      "name": "peak_ggH_ggHcat5",
  #      "title": "peak_ggH_ggHcat5",
  #      "type": "signal_shape",
  #      "mode": "other",
  #      "mean": "0.0",
  #      "sigma": "0.001",
  #      "cats": ["ggHcat5"],
  #  },
  #  {
  #      "name": "peak_ggH_ggHcat5",
  #      "title": "peak_ggH_ggHcat5",
  #      "type": "signal_shape",
  #      "mode": "other",
  #      "mean": "0.0",
  #      "sigma": "0.001",
  #      "cats": ["ggHcat6"],
  #  },
    {
        "name": "sigma_ggH_ggHcat1",
        "title": "sigma_ggH_ggHcat1",
        "type": "signal_shape",
        "mode": "other",
        "mean": "0.0",
        "sigma": "0.1",
        "cats": ["ggHcat1"],
    },
    {
        "name": "sigma_ggH_ggHcat2",
        "title": "sigma_ggH_ggHcat2",
        "type": "signal_shape",
        "mode": "other",
        "mean": "0.0",
        "sigma": "0.1",
        "cats": ["ggHcat2"],
    },
    {
        "name": "sigma_ggH_ggHcat3",
        "title": "sigma_ggH_ggHcat3",
        "type": "signal_shape",
        "mode": "other",
        "mean": "0.0",
        "sigma": "0.1",
        "cats": ["ggHcat3"],
    },
    {
        "name": "sigma_ggH_ggHcat4",
        "title": "sigma_ggH_ggHcat4",
        "type": "signal_shape",
        "mode": "other",
        "mean": "0.0",
        "sigma": "0.1",
        "cats": ["ggHcat4"],
    },
    {
        "name": "sigma_ggH_ggHcat5",
        "title": "sigma_ggH_ggHcat5",
        "type": "signal_shape",
        "mode": "other",
        "mean": "0.0",
        "sigma": "0.1",
        "cats": ["ggHcat5"],
    },
    {
        "name": "sigma_ggH_ggHcat6",
        "title": "sigma_ggH_ggHcat6",
        "type": "signal_shape",
        "mode": "other",
        "mean": "0.0",
        "sigma": "0.1",
        "cats": ["ggHcat6"],
    },
]
