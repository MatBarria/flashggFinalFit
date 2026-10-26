signal_colors = {"ggH": "red", "VBF": "blue", "ttH": "lime"}

def get_color_list(number_of_histograms):
    colors = [
        "#3f90da",
        "#ffa90e",
        "#bd1f01",
        "#94a4a2",
        "#832db6",
        "#a96b59",
        "#e76300",
        "#b9ac70",
        "#717581",
        "#92dadd",
    ]

    color_list = []
    for i in range(number_of_histograms):
        color_list.append(colors[i])

    return color_list
