
Code Notes:

1) Variable naming conventions.
g_Xxx - Global across multiple code modules
gXxx  - Global within the declaring code module

_Xxx  - Private/protected class variables



Build Notes:
   
   
Uncompressed file:
RETRIEVAL_POINTERS_BUFFER(0000000002521720) - ExtentCount:12 StartingVcn:0
	Extent:0        VCN:1                LCN:48613783         Clusters:1       
	Extent:1        VCN:2                LCN:51778515         Clusters:1       
	Extent:2        VCN:4                LCN:52639436         Clusters:2       
	Extent:3        VCN:8                LCN:52639689         Clusters:4       
	Extent:4        VCN:16               LCN:52686694         Clusters:8       
	Extent:5        VCN:32               LCN:52698205         Clusters:16      
	Extent:6        VCN:48               LCN:52639632         Clusters:16      
	Extent:7        VCN:96               LCN:51777368         Clusters:48      
	Extent:8        VCN:112              LCN:55540060         Clusters:16      
	Extent:9        VCN:128              LCN:12194306         Clusters:16      
	Extent:10       VCN:144              LCN:3744668          Clusters:16      
	Extent:11       VCN:1248             LCN:55613804         Clusters:1104    
	
Compressed file:
RETRIEVAL_POINTERS_BUFFER(0000000001E64AC0) - ExtentCount:28 StartingVcn:0
	Extent:0        VCN:1                LCN:55613373         Clusters:1
	Extent:1        VCN:16               LCN:-1               Clusters:15
	Extent:2        VCN:20               LCN:16859067         Clusters:4
	Extent:3        VCN:32               LCN:-1               Clusters:12
	Extent:4        VCN:40               LCN:12294799         Clusters:8
	Extent:5        VCN:48               LCN:-1               Clusters:8
	Extent:6        VCN:70               LCN:55537019         Clusters:22
	Extent:7        VCN:80               LCN:55536731         Clusters:10
	Extent:8        VCN:102              LCN:55540323         Clusters:22
	Extent:9        VCN:112              LCN:55538371         Clusters:10
	Extent:10       VCN:135              LCN:44087667         Clusters:23
	Extent:11       VCN:144              LCN:44179696         Clusters:9
	Extent:12       VCN:160              LCN:49979685         Clusters:16
	Extent:13       VCN:170              LCN:52639661         Clusters:10
	Extent:14       VCN:176              LCN:-1               Clusters:6
	Extent:15       VCN:187              LCN:40505666         Clusters:11
	Extent:16       VCN:192              LCN:-1               Clusters:5
	Extent:17       VCN:201              LCN:40505682         Clusters:9
	Extent:18       VCN:208              LCN:40761494         Clusters:7
	Extent:19       VCN:233              LCN:44087602         Clusters:25
	Extent:20       VCN:240              LCN:42089695         Clusters:7
	Extent:21       VCN:265              LCN:52639623         Clusters:25
	Extent:22       VCN:272              LCN:52640948         Clusters:7
	Extent:23       VCN:298              LCN:6320350          Clusters:26
	Extent:24       VCN:304              LCN:6542029          Clusters:6
	Extent:25       VCN:330              LCN:44179662         Clusters:26
	Extent:26       VCN:336              LCN:45618976         Clusters:6
	Extent:27       VCN:352              LCN:52642201         Clusters:16
Total clusters:352 Allocated clusters: 306

// find region for file

// walk retrieval pointers, move in/out as necessary.. each a new step on the list



// have something called a CostOfPlan field that is a value which defines how much of the file has to be moved
//  to accomodate the defrag plan. it could be a percentage of total file size, so say moving 15 clusters of
//  a file with 30 total clusters, the cost would be 50%. Another example would be a 30 cluster file that needs 
//  to be moved out and in, and as such more than 30 clusters would be moved. So say each cluster has to be moved
//  out then in, for a total of 60 clusters moved. The cost for this would be 200%.

// supporting x plan types..
//    full move to free space, always 100%
//    partial move to adjacent space, < 100%
//    consolidation in locally allocated space ~> 100%