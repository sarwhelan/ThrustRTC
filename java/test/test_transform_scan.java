import java.util.*;
import JThrustRTC.*;

public class test_transform_scan
{
	public static void main(String[] args) 
	{
		{
			DVVector d_data = new DVVector(new int[] { 1, 0, 2, 2, 1, 3 });
			TRTC.Transform_Inclusive_Scan(d_data, d_data, new Functor("Negate"), new Functor("Plus"));
			System.out.println(Arrays.toString((int[])d_data.to_host()));
		}

		{
			DVVector d_data = new DVVector(new int[] { 1, 0, 2, 2, 1, 3 });
			TRTC.Transform_Exclusive_Scan(d_data, d_data, new Functor("Negate"), new DVInt32(4), new Functor("Plus"));
			System.out.println(Arrays.toString((int[])d_data.to_host()));
		}
	}
}