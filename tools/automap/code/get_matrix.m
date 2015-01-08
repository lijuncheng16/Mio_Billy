function Matrix = get_matrix(data,method,paras)
% function to calculate the metric matrix for n sets of inputs, the metric
% is decided based on the method chosen from ITE toolbox
%
%   Inputs:
%
%     data   - n*p matrix for n sets of inputs with same dimension p; or 
%              n*1 cell for n sets of inputs with different dimension
%     method - the estimation method used to generae the matrix
%     paras  - cell structure, parameters including co and ds
%              paras.co: the object for the estimator
%              paras.ds: dimension for the inputs, default to be ones(2,1)
%
%   Outputs:
%
%     Metric_Matrix - n*n matrix showing the coresponding metric among n
%                     sets of inputs

if ~(exist('paras.ds'))
    paras.ds = ones(2,1);
end

n = size(data,1);
Matrix = zeros(n,n);

if iscell(data)
    n = length(data);
    Matrix = zeros(n,n);
    if size(data{1},1)>size(data{1},2)
        for i=1:n
            data{i} = data{i}';
        end
    end
    for i=1:n
        for j=1:n
%             if i~=j
            
                Matrix(i,j) = feval(method,data{i},data{j},paras.co);
%             end
        end
    end
    
else
    for i=1:n
        for j=1:n
            temp = data([i j],:);
            Matrix(i,j) = feval(method,temp,paras.ds,paras.co);
        end
    end
end

end