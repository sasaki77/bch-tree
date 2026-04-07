# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = "bch-tree"
copyright = "2026, Shinya Sasaki"
author = "Shinya Sasaki"
release = "0.1.0"

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = [
    "sphinx.ext.autosectionlabel",
    "sphinx.ext.githubpages",
    "sphinx_rtd_theme",
    "sphinx_multiversion",
    "myst_parser",
]

source_suffix = {
    ".rst": "restructuredtext",
    ".md": "markdown",
}

templates_path = ["_templates"]
exclude_patterns = [
    "_build",
    "Thumbs.db",
    ".DS_Store",
    ".venv",
]

myst_heading_anchors = 3

# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = "sphinx_rtd_theme"
html_theme_path = ["sphinx_rtd_theme.get_html_theme_path()"]
html_static_path = ["_static"]

html_context = {
    "display_github": True,
    "github_user": "sasaki77",
    "github_repo": "bch-tree",
    "github_version": "main/docs/",
}

github_doc_root = "https://github.com/sasaki77/bch-tree/main/doc/"

smv_tag_whitelist = r"^\d+\.\d+.*$"
smv_branch_whitelist = "main"
