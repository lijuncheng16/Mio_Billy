function Metrc_Matrix = get_metric_matrix(data,type,paras)
% function to calculate the metric matrix for n sets of inputs, the metric
% could be Pearson's linear correlation, Kendall's rank correlation,
% Spearman's rank correlation, mutual information, copula and etc.
%
%   Inputs:
%
%     data - n*p matrix for n sets of inputs with same dimension p; or 
%            n*1 cell for n sets of inputs with different dimension
%     type - 0(or default): Pearson's linear correlation
%            1: Kendall's rank correlation
%            2: Spearman's rank correlation
%     paras - cell structure, parameters for different types
%
%   Outputs:
%
%     Metric_Matrix - n*n matrix showing the coresponding metric among n
%                     sets of inputs




switch type
    
    case 0
        [Metric_Matrix,p] = corr(data');
        Metric_Matrix(p>0.05) = 0;
        
    case 1
        [Metric_Matrix,p] = corr(data','type','Kendall');
        Metric_Matrix(p>0.05) = 0;

    case 2
        [Metric_Matrix,p] = corr(data','type','Spearman');
        Metric_Matrix(p>0.05) = 0;


    otherwise
        disp('Wrong inputs for type, use default Pearson correlation')
        [Metric_Matrix,p] = corr(data');
        Metric_Matrix(p>0.05) = 0;
        
end

end