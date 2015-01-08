clear
%% base example 1 entropy estimation

Y = rand(5,1000);                         %generate the data of interest (d=5, T=1000)
mult = 1;                                 %multiplicative constant is important
co = HShannon_kNN_k_initialization(mult); %initialize the entropy (’H’) estimator
                                              %(’Shannon_kNN_k’), including the value of k
H = HShannon_kNN_k_estimation(Y,co);      %perform entropy estimation

%% base example 2 mutual information estimation

ds = [1;1;1]; Y = rand(sum(ds),5000);  % generate the data of interest
mult = 1;
co = IKCCA_initialization(mult);
I_kcca = IKCCA_estimation(Y,ds,co);

%% base example 3 distance correaltion

ds = [1;1]; Y = rand(sum(ds),5000);  % generate the data of interest
mult = 1;
co = IdCor_initialization(mult);
I_dCor = IdCor_estimation(Y,ds,co);

%% base example 4 KL divergence

Y1 = randn(3,2000); Y2 = randn(3,3000);
mult = 1;
co = DKL_kNN_k_initialization(mult);
D_KL = DKL_kNN_k_estimation(Y1,Y2,co);

%% base example 5 MMD-online

Y1 = randn(1,2000); Y2 = randn(1,2000);
mult = 1;
co = DMMD_online_initialization(mult);
D_MMD = DMMD_online_estimation(Y1,Y2,co);

%% base example 6 emprical copula

ds = ones(2,1); Y = rand(sum(ds),5000);
mult = 1;
co = ASpearman1_initialization(mult);
A = ASpearman1_estimation(Y,ds,co); 

%% matrix example 1 KL divergence
addpath('../../../Code')
Y = cell(3,1);
Y{1} = randn(1,2000); Y{2} = randn(1,3000); Y{3} = rand(1,5000); 
mult = 1;
paras.co = DKL_kNN_k_initialization(mult);
Matrix_D_KL = get_matrix(Y,@DKL_kNN_k_estimation,paras);

%% matrix example 2 emprical copula
paras.ds = ones(2,1); Y = rand(10,5000);
mult = 1;
paras.co = ASpearman1_initialization(mult);
Matrix_A = get_matrix(Y,@ASpearman1_estimation,paras); 
%% matrix example 3 MMD-online
Y = cell(3,1);
Y{1} = randn(1,2000); Y{2} = randn(1,3000); Y{3} = rand(1,5000); 
mult = 1;
paras.co = DMMD_online_initialization(mult);
Matrix_D_MMD = get_matrix(Y,@DMMD_online_estimation,paras);