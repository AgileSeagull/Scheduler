import pandas as pd
import matplotlib.pyplot as plt
import os
from matplotlib.patches import Patch


data_paths = {
    'DPS-DTQ': {
        'high_criticality_processes': 'outputs/DPS-DTQ/high_criticality_processes.csv',
        'simultaneous_arrival': 'outputs/DPS-DTQ/simultaneous_arrival.csv',
        'tight_deadlines': 'outputs/DPS-DTQ/tight_deadlines.csv'
    },
    'CFS': {
        'high_criticality_processes': 'outputs/CFS/high_criticality_processes.csv',
        'simultaneous_arrival': 'outputs/CFS/simultaneous_arrival.csv',
        'tight_deadlines': 'outputs/CFS/tight_deadlines.csv'
    },
    'REF_PAPER_ALGO': {
        'high_criticality_processes': 'outputs/REF_PAPER_ALGO/high_criticality_processes.csv',
        'simultaneous_arrival': 'outputs/REF_PAPER_ALGO/simultaneous_arrival.csv',
        'tight_deadlines': 'outputs/REF_PAPER_ALGO/tight_deadlines.csv'
    }
}


colors = {
    'DPS-DTQ': '#F8F9FA' ,       
    'CFS': '#ADB5BD',           
    'REF_PAPER_ALGO': '#495057'
}


plt.rcParams.update({
    'font.family': 'serif',
    'font.serif': ['Times New Roman', 'DejaVu Serif', 'Palatino Linotype'],
    'font.size': 11,
    'axes.labelsize': 12,
    'axes.titlesize': 13,
    'figure.titlesize': 14,
    'xtick.labelsize': 10,
    'ytick.labelsize': 10,
    'legend.fontsize': 10,
    'figure.dpi': 300,
    'savefig.dpi': 600,
})


data = {}
for algo, paths in data_paths.items():
    data[algo] = {}
    for case, path in paths.items():
        try:
            data[algo][case] = pd.read_csv(path)
        except Exception as e:
            print(f"Error loading {path}: {e}")
            continue


metrics = data['DPS-DTQ']['high_criticality_processes']['Metric'].tolist()


os.makedirs('plots', exist_ok=True)


for metric in metrics:
    fig, axes = plt.subplots(1, 3, figsize=(10, 5), sharey=True)
    fig.set_facecolor('white')  
    
    cases = ['high_criticality_processes', 'simultaneous_arrival', 'tight_deadlines']
    case_titles = ['High Criticality Processes', 'Simultaneous Arrival', 'Tight Deadlines']
    
    for i, (case, title) in enumerate(zip(cases, case_titles)):
        
        values = []
        algorithms = []
        bar_colors = []
        edge_colors = []
        
        for algo in ['DPS-DTQ', 'CFS', 'REF_PAPER_ALGO']:
            try:
                df = data[algo][case]
                value = df.loc[df['Metric'] == metric, 'Value'].values[0]
                values.append(value)
                algorithms.append('BTQRR' if algo == 'REF_PAPER_ALGO' else algo)
                bar_colors.append(colors[algo])
                edge_colors.append('black')  
            except Exception as e:
                print(f"Error processing {algo}/{case}/{metric}: {e}")
                continue

        
        if values:  
            
            bar_width = 0.6
            bars = axes[i].bar(algorithms, values, width=bar_width, color=bar_colors, 
                               edgecolor=edge_colors, linewidth=1.0)
            
            
            for j, bar in enumerate(bars):
                height = bar.get_height()
                text_color = 'white' if bar_colors[j] == 'black' else 'black'
                axes[i].text(bar.get_x() + bar.get_width()/2., height + 0.01*max(values) if values else 0,
                            f'{height:.2f}', ha='center', va='bottom', fontsize=9, color=text_color)
        
        
        axes[i].set_title(title, pad=10)
        if i == 0:
            axes[i].set_ylabel(metric, fontweight='bold')
        
        
        for spine in ['top', 'right']:
            axes[i].spines[spine].set_visible(False)
        
        axes[i].tick_params(axis='x', which='both', direction='out', length=4, width=1)
        axes[i].tick_params(axis='y', which='both', direction='out', length=4, width=1)
        
    
    legend_elements = [Patch(facecolor=colors[algo], edgecolor='black',
                            label='BTQRR' if algo == 'REF_PAPER_ALGO' else algo) 
                      for algo in ['DPS-DTQ', 'CFS', 'REF_PAPER_ALGO']]
    fig.legend(handles=legend_elements, loc='lower center', bbox_to_anchor=(0.5, -0.12), 
              ncol=3, frameon=True, edgecolor='black')
    
    
    fig.suptitle(f'{metric} Comparison', fontweight='bold', y=0.98)
    
    
    plt.subplots_adjust(left=0.08, right=0.98, top=0.88, bottom=0.20, wspace=0.12)
    
    
    
    base_metric = metric.split('(')[0].strip()
    plt.savefig(f'plots/{base_metric.replace(" ", "_").lower()}.png', dpi=600, bbox_inches='tight')
    plt.close()
    
    print(f"Plot for {metric} saved!")

print("All plots generated successfully!")
