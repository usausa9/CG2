float4 main(float4 pos : POSITION) : SV_POSITION
{
	/*return pos + float4(0.5f,0.5f,0,0);    右上に0.5fずつ移動 */ 
	/*return pos * float4(1.5f, 1.5f, 1, 1); 1.5倍 拡大 */
	return pos;
}